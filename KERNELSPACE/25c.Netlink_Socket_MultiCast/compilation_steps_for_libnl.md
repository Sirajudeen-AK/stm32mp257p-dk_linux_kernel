# Generic Netlink (libnl) Compilation Steps (WSL)

## 1. Update packages

``` bash
sudo apt update
```

## 2. Install required packages

``` bash
sudo apt install -y \
    build-essential \
    pkg-config \
    libnl-3-dev \
    libnl-genl-3-dev
```

## 3. Verify installation

``` bash
pkg-config --modversion libnl-3.0
pkg-config --modversion libnl-genl-3.0
pkg-config --cflags libnl-genl-3.0
pkg-config --libs libnl-genl-3.0
```

Expected include path:

``` text
-I/usr/include/libnl3
```

Verify header exists:

``` bash
ls /usr/include/libnl3/netlink/genl/genl.h
```

## 4. Compile

Recommended:

``` bash
gcc netlink_user_multicast_libnl.c \
    -o user_app \
    $(pkg-config --cflags --libs libnl-3.0 libnl-genl-3.0)
```

Equivalent manual command:

``` bash
gcc netlink_user_multicast_libnl.c \
    -I/usr/include/libnl3 \
    -lnl-3 \
    -lnl-genl-3 \
    -o user_app
```

## 5. Run

``` bash
./user_app
```

## Common Errors

### fatal error: netlink/genl/genl.h: No such file or directory

Cause: - `libnl-genl-3-dev` is not installed, or - include path is
missing.

Check:

``` bash
find /usr/include -name genl.h
```

### pkg-config cannot find libnl

Ensure:

``` bash
pkg-config --list-all | grep libnl
```

### Undefined reference errors

Compile using:

``` bash
$(pkg-config --cflags --libs libnl-3.0 libnl-genl-3.0)
```

instead of only `-lnl-3 -lnl-genl-3`.

## Useful Diagnostics

``` bash
pkg-config --modversion libnl-3.0
pkg-config --modversion libnl-genl-3.0
pkg-config --cflags libnl-genl-3.0
pkg-config --libs libnl-genl-3.0
find /usr/include -name genl.h
dpkg -L libnl-genl-3-dev | grep genl.h
```
