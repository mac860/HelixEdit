# HelixEdit 0.1.9 — APT sandbox-compatible local installation

## Why Ubuntu displayed a notice

The installation output shows that the package upgrade finished:

```
Unpacking helixedit (0.1.8) over (0.1.7)…
Setting up helixedit (0.1.8)…
```

APT then printed:

```
Notice: Download is performed unsandboxed as root as file
'/home/apple/Downloads/v3/HelixEdit/build-ubuntu-amd64/HelixEdit-0.1.8-x86_64.deb'
couldn't be accessed by user '_apt'. - pkgAcquire::Run (13: Permission denied)
```

The `_apt` sandbox account cannot traverse an ancestor of that package path, generally because the user's home/source folders restrict access to other system users. APT can still perform installation by falling back to root. This is an **informational security warning, not a HelixEdit installation failure**. The other output (`105 Not Upgrading`) is unrelated to this package.

## New safe installation path

`scripts/install_ubuntu_deb.sh` is a user-executed installer that:

1. Accepts an explicit `.deb` path or discovers the package for the current `CMakeLists.txt` version and CPU architecture.
2. Verifies the Debian package name `helixedit` and the host's Debian architecture before elevating privileges.
3. Creates `/tmp/helixedit-apt.XXXXXXXX` using `mktemp -d`; sets the directory to `0755` and stages the package as a `0644` file.
4. Invokes `sudo apt-get install --reinstall <staged.deb>` or, when called by the in-app source updater, `pkexec /usr/bin/apt-get install --reinstall -y <staged.deb>`.
5. Removes the staging folder on all ordinary exit paths through a shell `EXIT` trap.

This gives `_apt` permission to read the package **without changing the home directory, Git checkout, or APT sandbox user**. Build and test always run unprivileged. Only the final system install is elevated.

## Usage on Ubuntu 26.04 x86_64

```bash
cd ~/Downloads/v3/HelixEdit
git pull --ff-only origin main
bash scripts/build_ubuntu_deb.sh
bash scripts/install_ubuntu_deb.sh
```

To choose an already-built DEB explicitly:

```bash
bash scripts/install_ubuntu_deb.sh ./build-ubuntu-amd64/HelixEdit-0.1.9-x86_64.deb
```

**Do not** run the installer helper itself with `sudo`, give `_apt` ownership of the source, `chmod -R 777` any user files, or set `APT::Sandbox::User "root"` to suppress the warning.

## In-app updates

The existing official-source updater (`scripts/update_from_source.sh`) now delegates its final install to `scripts/install_ubuntu_deb.sh --pkexec`. It continues to check the Git origin and branch, insist on a clean checkout, fast-forward, build, and run CTest **before** requesting installation privileges. No background updates or schedules are added.

Users currently on HelixEdit 0.1.8 can perform one terminal upgrade using the commands above. After 0.1.9 is installed, the bundled in-app updater uses the same sandbox-friendly installation process.

## Validation

- `scripts/test_install_staging.sh <actual.deb>` mocks the `sudo` command without elevation, verifies the staged package's name and `0755`/`0644` permissions, and confirms cleanup.
- `scripts/build_ubuntu_deb.sh` runs this test automatically after CPack verifies the DEB and its contents.
- The GitHub build workflow runs `bash -n` over the installer and smoke-test scripts.
- Full native Ubuntu 26.04 packaging and installation of 0.1.9 have **not yet been independently confirmed**.
