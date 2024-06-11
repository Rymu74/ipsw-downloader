

# :3

json_generator is only used if you are planning to host the json with the download links. i have them on my server\
so dont worry abt this file

**dependencies**

debian\
```sudo apt install libcurl4-openssl-dev libjson-c-dev```

arch\
```sudo pacman -S libcurl json-c```

macos\
```brew install json-c```


\
**compiling**

linux\
```cc main.c -o ipsw -lcurl -ljson-c```

macos\
```clang -o ipsw main.c -I/opt/homebrew/opt/json-c/include -L/opt/homebrew/opt/json-c/lib -ljson-c -lcurl```

\
**installing**

linux\
```sudo cp ipsw /usr/bin```

macos\
```sudo cp ipsw /opt/homebrew/bin```
