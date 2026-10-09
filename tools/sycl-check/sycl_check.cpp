// DarkStudio - sycl-check: verify the SYCL toolchain and measure oneMKL SGEMM on the GPU (M0b).
// SPDX-License-Identifier: Apache-2.0
//
// Build (from a oneAPI-enabled prompt; on Windows icx uses MSVC-style options):
//   Windows: icx -fsycl /O2 /std:c++20 /EHsc /Qmkl sycl_check.cpp /Fe:sycl-check.exe
//   Linux:   icpx -fsycl -O2 -std=c++20 -qmkl sycl_check.cpp -o sycl-check
//
// Darknet's GPU path runs convolutions as im2col + SGEMM, so SGEMM speed on the device is a good
// first estimate of how fast Darknet training can be once it is ported to SYCL.

#include <sycl/sycl.hpp>
#include <oneapi/mkl/blas.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <random>
#include <vector>

namespace
{
	void show_device(const sycl::device & d)
	{
		const auto sub_groups = d.get_info<sycl::info::device::sub_group_sizes>();
		std::cout
			<< "device              " << d.get_info<sycl::info::device::name>() << std::endl
			<< "backend             " << d.get_backend() << ", driver " << d.get_info<sycl::info::device::driver_version>() << std::endl
			<< "compute units       " << d.get_info<sycl::info::device::max_compute_units>() << std::endl
			<< "max work-group size " << d.get_info<sycl::info::device::max_work_group_size>() << std::endl
			<< "sub-group sizes     ";
		for (const auto s : sub_groups)
		{
			std::cout << s << " ";
		}
		std::cout << std::endl
			<< "local memory        " << d.get_info<sycl::info::device::local_mem_size>() / 1024 << " KiB" << std::endl
			<< "global memory       " << d.get_info<sycl::info::device::global_mem_size>() / (1024 * 1024) << " MiB" << std::endl
			<< "fp16                " << (d.has(sycl::aspect::fp16) ? "yes" : "no") << std::endl
			<< "fp64                " << (d.has(sycl::aspect::fp64) ? "yes" : "no") << std::endl;
	}

	/// Darknet launches most kernels with a block size of 512 (BLOCK in dark_cuda.hpp).
	bool test_kernel_512(sycl::queue & q)
	{
		constexpr size_t n = 512 * 1024;
		float * a = sycl::malloc_shared<float>(n, q);
		for (size_t i = 0; i < n; i ++)
		{
			a[i] = static_cast<float>(i);
		}
		q.parallel_for(sycl::nd_range<1>(n, 512), [=](sycl::nd_item<1> it)
		{
			const size_t i = it.get_global_id(0);
			a[i] = a[i] * 2.0f + 1.0f;
		}).wait();

		bool ok = true;
		for (size_t i = 0; i < n; i ++)
		{
			if (a[i] != static_cast<float>(i) * 2.0f + 1.0f)
			{
				ok = false;
				break;
			}
		}
		sycl::free(a, q);
		return ok;
	}

	/// Row-major C = A * B with oneMKL, compared against a naive CPU result for a small size.
	void bench_sgemm(sycl::queue & q, const int m, const int n, const int k, const int reps)
	{
		std::vector<float> ha(static_cast<size_t>(m) * k), hb(static_cast<size_t>(k) * n), hc(static_cast<size_t>(m) * n);
		std::mt19937 rng(42);
		std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
		std::generate(ha.begin(), ha.end(), [&] { return dist(rng); });
		std::generate(hb.begin(), hb.end(), [&] { return dist(rng); });

		float * a = sycl::malloc_device<float>(ha.size(), q);
		float * b = sycl::malloc_device<float>(hb.size(), q);
		float * c = sycl::malloc_device<float>(hc.size(), q);
		q.memcpy(a, ha.data(), ha.size() * sizeof(float));
		q.memcpy(b, hb.data(), hb.size() * sizeof(float)).wait();

		using oneapi::mkl::transpose;
		auto run = [&]
		{
			return oneapi::mkl::blas::row_major::gemm(q, transpose::nontrans, transpose::nontrans, m, n, k, 1.0f, a, k, b, n, 0.0f, c, n);
		};

		run().wait();	// warm-up: JIT and kernel selection
		const auto start = std::chrono::steady_clock::now();
		for (int i = 0; i < reps; i ++)
		{
			run();
		}
		q.wait();
		const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count() / reps;
		const double gflops = 2.0 * m * n * k / (ms * 1.0e6);

		q.memcpy(hc.data(), c, hc.size() * sizeof(float)).wait();

		// spot-check a few elements against the CPU
		double max_err = 0.0;
		for (int s = 0; s < 16; s ++)
		{
			const int row = (s * 7919) % m;
			const int col = (s * 104729) % n;
			double ref = 0.0;
			for (int i = 0; i < k; i ++)
			{
				ref += static_cast<double>(ha[static_cast<size_t>(row) * k + i]) * hb[static_cast<size_t>(i) * n + col];
			}
			max_err = std::max(max_err, std::abs(ref - hc[static_cast<size_t>(row) * n + col]));
		}

		std::cout << "  sgemm " << std::setw(5) << m << " x " << std::setw(5) << n << " x " << std::setw(5) << k
			<< std::fixed << std::setprecision(2) << "  " << std::setw(8) << ms << " ms  "
			<< std::setprecision(0) << std::setw(5) << gflops << " GFLOPS  max err " << std::scientific << std::setprecision(1) << max_err
			<< std::defaultfloat << std::endl;

		sycl::free(a, q);
		sycl::free(b, q);
		sycl::free(c, q);
	}
}


int main()
{
	try
	{
		for (const auto & selector : {sycl::gpu_selector_v, sycl::cpu_selector_v})
		{
			sycl::queue q(selector, sycl::property::queue::in_order());
			std::cout << std::endl;
			show_device(q.get_device());
			std::cout << "kernel, wg=512      " << (test_kernel_512(q) ? "OK" : "FAILED") << std::endl;

			// sizes typical of yolov4-tiny convolutions at 416x416 (M = filters, N = out_h*out_w, K = size*size*channels)
			bench_sgemm(q,   64, 43264,   288, 10);	// 3x3x32 -> 64, 208x208
			bench_sgemm(q,  128, 10816,   576, 10);	// 3x3x64 -> 128, 104x104
			bench_sgemm(q,  256,  2704,  1152, 10);	// 3x3x128 -> 256, 52x52
			bench_sgemm(q,  512,   676,  2304, 10);	// 3x3x256 -> 512, 26x26
			bench_sgemm(q, 1024,  1024,  1024, 10);	// square reference
			bench_sgemm(q, 2048,  2048,  2048,  5);	// square reference
		}
	}
	catch (const std::exception & e)
	{
		std::cerr << "error: " << e.what() << std::endl;
		return 1;
	}
	return 0;
}
