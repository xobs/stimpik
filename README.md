# Stimpak

Extract firmware from an STM32F4 that has been locked with RDP1.

This attack uses a Raspberry Pi Pico.

## Building

```
git submodule update --init
cmake -B build -DPICO_SDK_PATH=path-to-pico-sdk
make -C build
```
