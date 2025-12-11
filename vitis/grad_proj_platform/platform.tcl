# 
# Usage: To re-create this platform project launch xsct with below options.
# xsct /home/jasonwang/Desktop/ece253/grad_proj/vitis/grad_proj_platform/platform.tcl
# 
# OR launch xsct and run below command.
# source /home/jasonwang/Desktop/ece253/grad_proj/vitis/grad_proj_platform/platform.tcl
# 
# To create the platform in a different location, modify the -out option of "platform create" command.
# -out option specifies the output directory of the platform project.

platform create -name {grad_proj_platform}\
-hw {/home/jasonwang/Desktop/ece253/grad_proj/vivado/grad_proj_hw_platform.xsa}\
-proc {microblaze_0} -os {standalone} -out {/home/jasonwang/Desktop/ece253/grad_proj/vitis}

platform write
platform generate -domains 
platform active {grad_proj_platform}
platform clean
platform generate
platform active {grad_proj_platform}
bsp reload
bsp config stdin "mdm_1"
bsp config stdout "mdm_1"
bsp write
platform generate -domains 
platform generate -domains 
platform active {grad_proj_platform}
platform config -updatehw {/home/jasonwang/Desktop/ece253/grad_proj/vivado/grad_proj_pwm_test.xsa}
platform clean
platform generate
platform clean
platform generate
platform config -updatehw {/home/jasonwang/Desktop/ece253/grad_proj/vivado/grad_proj_hw_platform.xsa}
platform clean
platform generate
