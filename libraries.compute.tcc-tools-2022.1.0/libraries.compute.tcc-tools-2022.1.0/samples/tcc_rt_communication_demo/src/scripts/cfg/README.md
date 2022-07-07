## Configurations
The tcc_rt_communication_demo uses configuration files in JSON format to define execution parameters
for each supported profile and stored in `cfg/` directory. The configuration files are grouped by
the target HW (Elkhart_Lake/Tiger_Lake_U/Tiger_Lake_H), the role in the sample
(talker/listerner/monitor/compute) and whether the profile uses optimized or non-optimized
execution parameters (opt/noopt).

The configuration files for the `basic` profiles are stored in the following files:
* cfg/basic/Elkhart_Lake/talker-opt.json
* cfg/basic/Elkhart_Lake/talker-noopt.json
* cfg/basic/Elkhart_Lake/listener-opt.json
* cfg/basic/Elkhart_Lake/listener-noopt.json
* cfg/basic/Tiger_Lake_U/talker-opt.json
* cfg/basic/Tiger_Lake_U/talker-noopt.json
* cfg/basic/Tiger_Lake_U/listener-opt.json
* cfg/basic/Tiger_Lake_U/listener-noopt.json
* cfg/basic/Tiger_Lake_H/talker-opt.json
* cfg/basic/Tiger_Lake_H/talker-noopt.json
* cfg/basic/Tiger_Lake_H/listener-opt.json
* cfg/basic/Tiger_Lake_H/listener-noopt.json

The configuration files for the `siso-single` profiles are stored in the following files:
* cfg/siso-single/Elkhart_Lake/monitor-opt.json
* cfg/siso-single/Elkhart_Lake/monitor-noopt.json
* cfg/siso-single/Elkhart_Lake/compute-opt.json
* cfg/siso-single/Elkhart_Lake/compute-noopt.json
* cfg/siso-single/Tiger_Lake_U/monitor-opt.json
* cfg/siso-single/Tiger_Lake_U/monitor-noopt.json
* cfg/siso-single/Tiger_Lake_U/compute-opt.json
* cfg/siso-single/Tiger_Lake_U/compute-noopt.json
* cfg/siso-single/Tiger_Lake_H/monitor-opt.json
* cfg/siso-single/Tiger_Lake_H/monitor-noopt.json
* cfg/siso-single/Tiger_Lake_H/compute-opt.json
* cfg/siso-single/Tiger_Lake_H/compute-noopt.json

## Legal Information

© Intel Corporation​. Intel, the Intel logo, and other Intel marks are trademarks of Intel Corporation or its subsidiaries. Other names and brands may be claimed as the property of others.

Copyright Intel Corporation
