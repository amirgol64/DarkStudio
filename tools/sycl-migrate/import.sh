#!/bin/bash
# DarkStudio - import SYCLomatic output into the darknet fork as src-lib/sycl/*.dp.cpp (M0b).
# SPDX-License-Identifier: Apache-2.0
#
# Usage: bash tools/sycl-migrate/import.sh [migration-dir]
#
# Replaces SYCLomatic's dpct helpers with the small dn_sycl:: equivalents in darknet_sycl.hpp, so the SYCL backend
# has no dependency on the dpct headers.  This is the *initial* import: after it, the files in src-lib/sycl/ are
# maintained by hand (fp32 literals, review fixes, ...), so re-running it overwrites those changes.
set -eo pipefail

here="$(cd "$(dirname "$0")" && pwd)"
root="$(cd "$here/../.." && pwd)"
in="${1:-$root/build/sycl-migration}"
out="$root/darknet/src-lib/sycl"
mkdir -p "$out"

for f in "$in"/*.dp.cpp; do
	name="$(basename "$f")"
	cu="${name%.dp.cpp}.cu"
	{
		echo "/* Darknet/YOLO:  https://codeberg.org/CCodeRun/darknet"
		echo " * SYCL version of src-lib/$cu, generated with SYCLomatic and adapted by the DarkStudio project."
		echo " * SPDX-License-Identifier: Apache-2.0"
		echo " */"
		echo
		sed -E \
			-e '/^#include <dpct\/.*>/d' \
			-e '/^#define DPCT_COMPAT_RT_VERSION /d' \
			-e 's/\bdpct::/dn_sycl::/g' \
			-e 's/\bDPCT_CHECK_ERROR\b/DN_SYCL_CHECK_ERROR/g' \
			-e 's/\bDPCT_COMPAT_RT_VERSION\b/DN_SYCL_COMPAT_RT_VERSION/g' \
			-e 's/\bDPCT_COMPATIBILITY_TEMP\b/DN_SYCL_CUDA_ARCH/g' \
			-e 's/CHECK_CUDA\(0\)/CHECK_CUDA(cudaPeekAtLastError())/g' \
			-e 's/\b__popc\(/sycl::popcount(/g' \
			-e 's/\r$//' \
			"$f" |
		# drop the comments for warnings that are resolved by darknet_sycl.hpp:
		#   DPCT1010 (cudaPeekAtLastError replaced with 0) and DPCT1049 (work-group size; BLOCK=512 is supported)
		perl -0pe 's{[ \t]*/\*\s*\n?\s*DPCT10(10|49):\d+:.*?\*/[ \t]*\n}{}gs' |
		# Kernel lambdas capture by copy ([=]).  When the body uses "l.w" or "state.input", the whole Darknet::Layer
		# or NetworkState struct is copied into the kernel arguments, which exceeds the 2 KiB kernel argument limit
		# of Intel GPUs.  Capture only the fields that are used, e.g. [=, l_w = l.w](...) { ... l_w ... }.
		perl -0pe '
			s{\[=\](\(sycl::nd_item<3>\s+item_ct1\)(?:\s*\[\[[^\]]*\]\])?\s*)(\{(?:[^{}]++|(?2))*\})}{
				my ($sig, $body) = ($1, $2);
				my (%seen, @captures);
				$body =~ s/\b(l|state|net|layer)\.(\w+)/
					my $name = "$1_$2";
					push @captures, "$name = $1.$2" unless $seen{$name}++;
					$name
				/ge;
				@captures ? "[=, " . join(", ", @captures) . "]" . $sig . $body : "[=]" . $sig . $body
			}gse'
	} > "$out/$name"
	echo "imported $name"
done

leftover="$(grep -l 'dpct' "$out"/*.dp.cpp || true)"
if [ -n "$leftover" ]; then
	echo "files still mentioning dpct (comments are fine):"
	grep -n 'dpct' "$out"/*.dp.cpp | grep -v 'DPCT[0-9]' | head -20 || true
fi
