# DarkStudio

DarkStudio is a free, open-source computer-vision toolchain. It lets you label, train, run inference and export from one web app. It is built on [Darknet](https://codeberg.org/CCodeRun/darknet) and [DarkHelp](https://codeberg.org/CCodeRun/DarkHelp).

> Status: early planning. See [plan.md](plan.md) for the roadmap.

## Repository layout

| Path | What it is | License |
|---|---|---|
| `darknet/` | Submodule: fork of Hank.ai Darknet (training and inference engine) | Apache-2.0 |
| `DarkHelp/` | Submodule: fork of DarkHelp (inference API) | MIT |
| `DarkMark/` | Submodule: fork of DarkMark, **kept for reference only and not built into DarkStudio** | GPL-3.0 |
| `server/` | C++ backend (Drogon). Planned | Apache-2.0 |
| `web/` | React + TypeScript web UI. Planned | Apache-2.0 |

## Getting the code

```sh
git clone --recurse-submodules https://github.com/amirgol64/DarkStudio.git
```

If you already cloned without submodules:

```sh
git submodule update --init
```

## Syncing the forks with upstream

Each submodule has two remotes. `origin` is the GitHub fork, where you push your work. `upstream` is the original project on Codeberg, and it is fetch-only.

```sh
cd darknet
git fetch upstream
git rebase upstream/master   # or: git merge upstream/master
git push origin master       # use --force-with-lease after a rebase
cd ..
git add darknet && git commit -m "Bump darknet"
```

A fresh clone only has `origin`. Add the upstream remote once per submodule:

```sh
git -C darknet  remote add upstream https://codeberg.org/CCodeRun/darknet.git
git -C DarkHelp remote add upstream https://codeberg.org/CCodeRun/DarkHelp.git
git -C DarkMark remote add upstream https://codeberg.org/CCodeRun/DarkMark.git
```

## License

DarkStudio's own code is licensed under [Apache-2.0](LICENSE). Each submodule keeps its own license. The planned model registry marks every model with its license, and models that are AGPL or not allowed for commercial use are offered only as optional plugins. See [plan.md](plan.md#2-licensing-policy).
