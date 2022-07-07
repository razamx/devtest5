# HOWTO:
## Run host tests with valgrind:

```bash
cd tccsdk
mkdri build
cd build
cmake -DCMAKE_INSTALL_PREFIX=install ../
TCC_RUN_PREFIX="valgrind --leak-check=full --track-origins=yes --track-fds=yes --error-exitcode=1 --num-callers=128 --trace-children=yes --error-limit=no --show-reachable=yes --suppressions=<path/to/suppressions>" make run_host_tests
```

## Run target bats tests with valgrind:

```bash
export TCC_RUN_PREFIX='valgrind --leak-check=full --track-origins=yes --track-fds=yes --error-exitcode=1 --num-callers=128 --trace-children=yes --log-file=log_%p.txt --error-limit=no --show-reachable=yes --suppressions=<path/to/suppressions>'
bats <file_name>
```

## Run target unit tests with valgrind:

```bash
valgrind --leak-check=full --track-origins=yes --track-fds=yes --error-exitcode=1 --num-callers=128 --trace-children=yes --error-limit=no --show-reachable=yes --suppressions=<path/to/suppressions> <file_name>
```

## Generate suppressions

If you want to generate suppressions - add follow parameter to valgrind command
```bash
--gen-suppressions=all
```

# Naming rules:

1. Find or create block with file name where issue was found. As example:

`#############################`<br/>
`##### test_new_api_host #####`<br/>
`#############################`<br/>

2. For each new suppression add comment with description started with issue type:
    * [T] - for test issue
    * [P] - for product issue
    * [S] - for samples issue
    * [TP] - for third party issue (as python default libs, stress-ng and others)

3. For product issues (types T, P, S) add bug/us number with attached log:

    As example:  `#[T][DE8256] Call add_node without free (buffer.c:597) (call_add_node_without_free)`

