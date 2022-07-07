# Git hooks

Git hooks allow to run script on different git stages (e.g. before push).
If hook fails, action won't be performed.

## How to use
1. Clone `tccdev`
2. Add `<tccdev>/tools` directory to the `PATH`
3. Create a symbolic link to the hooks you need from this directory to the
`<tccsdk>/.git/hooks`, e.g.
```
ln -s <tccdev>/git-hooks/pre-push <tccsdk>/.git/hooks/.
```
4. Make it executable, e.g.
```
chmod +x <tccsdk>/.git/hooks/pre-push
```
5. Profit!

## Hooks available
### pre-push
Run rules checker (check copyright header) for all files, which changed from master.
