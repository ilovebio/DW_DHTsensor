# DW_DHTsensor
4Y30 Demi Wang
In this project, I connected four peripherals at once: a DHT-22, an SSD1306 OLED, a button, and a WT3000 voice module. During the process, I learned different things, faced a variety of obstacles, and made mistakes.

Firstly, I learned a lot about wiring. When making the prototype on a breadboard, I found that the DHT sensor wasn't giving out data. After debugging, I found that it was because I connected the pins incorrectly, as I had used the way I was used to connecting the DHT-22, since the pin layout wasn't the same as the DHT-22 I used to use. It also served as a reminder that I should keep in mind when doing further projects—not to be overconfident and rely too much on "previous" experience.

Secondly, when I was adding the button, I found that when I pressed the button once, the voice module kept repeatedly announcing the temperature and humidity. That was when I was suddenly reminded that the button needs debouncing, and I had to do it with `millis()` instead of `delay()` to keep the rest of the program responsive.

All in all, this project trained my software and wiring skills. The feeling of finishing a project and solving all the problems (with help from others, of course) is something I found precious and can never forget.
