To compile this project, first download the submodules:

```
git submodule update --init --recursive
```

This command will download the git repo.

Then follow ths standard CMake procedure to compile this project:

```
mkdir build
cd build
cmake ../
make
```

If you want to always use the up-to-date version of the submodules, please run the following command to update the module:

```
git submodule update --remote --recursive
```
