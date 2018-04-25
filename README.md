# NS3-Directional-Antenna

## Descprition
This is a patch of the NS3 Network simulator [https://www.nsnam.org/](https://www.nsnam.org/).
It adds a directional antennas module to the WIFI module of the NS3 simulator
The directional antenna model we developed is based on the following
work: “Efﬁcient Antenna Patterns for Three-Sector WCDMA Systems”.

## Version
ns-3.14.1

## How to
Replace the corresponding files by those in src.
The file “wscript” should be placed in this folder “ns-allinone-3.14.1\ns-3.14.1\src\wifi”.

wifi80211g.cc is an example on how to
create an ad-hoc network with directed antennas. 
