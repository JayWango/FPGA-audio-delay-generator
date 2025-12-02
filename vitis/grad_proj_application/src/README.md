# Issue Log:
1. Needed a way to test if the PWM signal to the audio output was working  

## Solution: 
- Modify the constraints (.xdc) file by remapping pin H17 (corresponds to LED 0 on the FPGA board) to the 'audio_pwm_output' signal.
- Remap pin A11 (the actual pwm output signal) to LED 0 temporarily for testing purposes, and to avoid errors when generating bitstream.
- Regenerate the bitstream in Vivado, then change the hardware specification of the platform project in Vitis to the newly generated .xsa file, clean & rebuild platform project, clean & rebuild the application project. 
- Make sure to set the options of the PWM timer (axi_timer_1) for both channels 0 and 1 to have down-counting and auto-reload on. 

*Note:* In order to test the PWM signal, we needed to regenerate a new .xsa file called 'grad_proj_pwm_test.xsa' in the /vivado/ directory. 
To revert back to the proper wiring of the PWM signal to the audio output, we need to clean & rebuild our platform project with the original 'grad_proj_hw_platform.xsa' file.

2. After verifying that the PWM signal works, we noticed that every time we tapped or blew on the mic, we could hear something from the exciter but it just sounded like a loud "pop". 
We printed out the values of pwm_sample to see the actual number being sent to channel 1 of the PWM timer as the "reset value". 
By analyzing our sampling_ISR(), it seems that the pwm_sample values exceed RESET_VALUE (set as 2267), so our audio is just constantly clipping. 


## Solution:
- 