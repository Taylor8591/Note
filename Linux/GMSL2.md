- packet-based
- Typical GMSL2 devices have a forward serial bit rate of 3Gbps or 6Gbps and reverse channel serial bit rate of 187.5Mbps.
# GMSL2 Link Configurations
## Single-Link Mode
![[Pasted image 20251108100720.png]]
## Reverse Splitter Mode
![[Pasted image 20251108100728.png]]
## Dual-Link Mode
![[Pasted image 20251108100859.png]]

# GMSL2 Link Rate and Configuration Programming
## Programming Link Rate
- The link rate is configured with the TX_RATE[1:0] and RX_RATE[1:0] registers in each device.
## Programming Link Configuration
### Auto Link Mode
Link configuration is controlled with the AUTO_LINK and LINK_CFG[1:0] registers. After link configuration settings are changed, reset the link for the changes to take effect.

By default, the automatic link configuration mode is enabled (AUTO_LINK = 1). This means the device attempts to lock in ==single-link mode==. It automatically detects if PHYA or PHYB is connected to the remote device and enables that PHY. If both PHYA and PHYB are connected to remote devices, it connects in single-link mode to whichever device is first detected.

Note: In auto link mode, the LINK_CFG[1:0] setting is ignored.
### Manual Link Mode
### Standard Splitter Mode: Switch from Splitter Mode to Single-Link Mode
### Standard Splitter Mode: Switch from Single-Link Mode to Splitter Mode
## GMSL2 Link Lock
## Video Lock
Video lock indicates that the deserializer is receiving valid video data from the serializer. After the  
GMSL2 link has locked, the deserializer video lock sequence begins. Optionally, the LOCK pin behavior  
in the deserializer can be changed by a register setting (LOCK_CFG) so that the LOCK pin is asserted  
only when the deserializer is receiving video (asserted with VIDEO_LOCK).
# Resets
## Reset All
## Oneshot Reset
## Reset Link
# Video Pipes
The GMSL2 video channel is designed to transmit 8-bit to 24-bit video data. Video data is received by  
the serializer at the input video interface (example, HDMI), converted to parallel video data, and  
transmitted through the serializer video pipes to the GMSL packetizer. The data is then packetized and  
transmitted by the GMSL PHY across the serial link. The connected deserializer receives the data on  
the GMSL PHY, and the data is depacketized into parallel video data. The parallel video data is  
transmitted through the video pipes to the output video interface (example, oLDI). Within the GMSL  
parts, the parallel video data consists of the pixel data and HS, VS, and DE sync signals![[Pasted image 20251108104432.png]]