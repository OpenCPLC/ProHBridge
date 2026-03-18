## 🌉 HBridge

PWM H-bridge control via CMD console on STM32G081 _(Nucleo)_.

### 🚀 Run

```sh
pip install opencplc # install Forge
opencplc -g https://github.com/OpenCPLC/ProHBridge # download project
make # build
make run # build & flash
```

### 📟 Cli

Connect via **UART** _(**115200**bps 8N1)_ and send commands:

```sh
1000Hz 50% 4us ON # set freq, duty, deadtime and enable
500Hz # change only frequency
75% # change only duty cycle
OFF # disable H-bridge
ON # enable H-bridge
0us # disable dead time
```

| Argument       | Description                          |
| -------------- | ------------------------------------ |
| `<val>Hz`      | Frequency _(e.g. `1000Hz`, `20kHz`)_ |
| `<val>%`       | Duty cycle _(e.g. `50%`, `10%`)_     |
| `<val>s/ms/us` | Dead time _(e.g. `2.5us`, `700ns`)_  |
| `ON` / `Start` | Enable H-bridge output               |
| `OFF` / `Stop` | Disable H-bridge output              |

Arguments can be combined or sent individually. Current config is applied immediately.

## 🔗 Connections

```
PA8 → CH1      PA7 → CH1N
PA9 → CH2      PB0 → CH2N
PA6 → enable1  PA4 → enable2
PA5 → LED      (heartbeat)
PC4 → uart-TX  PC5 → uart-RX
```