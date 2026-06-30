# Running LVGL on M5Stick S3

This repository contains an example of LVGL running on M5Stick S3. Running LVGL with the setup/loop framework is usually not the best idea, mainly if you want to save power in an interactive device. So this example uses FreeRTOS (included in the M5Stick S3 environment by default) timers and queues to achieve that. It also supports LVGL's `indev` devices using "event mode".

# Programming Environment

This is a PlatformIO project, it depends on M5Unified and LVGL 9.5.
