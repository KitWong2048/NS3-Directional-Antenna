// 
// This script configures two nodes on an 802.11b physical layer, with
// 802.11b NICs in adhoc mode, and by default, sends one packet of 1000 
// (application) bytes to the other node.  The physical layer is configured
// to receive at a fixed RSS (regardless of the distance and transmit
// power); therefore, changing position of the nodes has no effect. 
//
// There are a number of command-line options available to control
// the default behavior.  The list of available command-line options
// can be listed with the following command:
// ./waf --run "wifi-simple-adhoc --help"
//
// For instance, for this configuration, the physical layer will
// stop successfully receiving packets when rss drops below -97 dBm.
// To see this effect, try running:
//
// ./waf --run "wifi-simple-adhoc --rss=-97 --numPackets=20"
// ./waf --run "wifi-simple-adhoc --rss=-98 --numPackets=20"
// ./waf --run "wifi-simple-adhoc --rss=-99 --numPackets=20"
//
// Note that all ns-3 attributes (not just the ones exposed in the below
// script) can be changed at command line; see the documentation.
//
// This script can also be helpful to put the Wifi layer into verbose
// logging mode; this command will turn on all wifi logging:
// 
// ./waf --run "wifi-simple-adhoc --verbose=1"
//
// When you are done, you will notice two pcap trace files in your directory.
// If you have tcpdump installed, you can try this:
//
// tcpdump -r wifi-simple-adhoc-0-0.pcap -nn -tt
//

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/antenna-model.h"
#include "ns3/cosine-antenna-model.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/applications-module.h"
#define SIM_TIME 21
NS_LOG_COMPONENT_DEFINE ("WifiSimpleAdhoc");

using namespace ns3;

void ReceivePacket (Ptr<Socket> socket)
{
  //NS_LOG_UNCOND ("Received one packet!");
  //printf("%s\n\n","Received one packet!");
}

static void GenerateTraffic (Ptr<Socket> socket, uint32_t pktSize, 
                             uint32_t pktCount, Time pktInterval )
{
  if (pktCount > 0)
    {
      socket->Send (Create<Packet> (pktSize));
      Simulator::Schedule (pktInterval, &GenerateTraffic, 
                           socket, pktSize,pktCount-1, pktInterval);
    }
  else
    {
      socket->Close ();
    }
}


void UDP_Setup (ApplicationContainer& apps ,Ptr<Node> source, Ptr<Node> dest, double rate, uint16_t port){    
    Address sinkLocalAddress(dest->GetObject<Ipv4>()->GetAddress(1,0).GetLocal());
    PacketSinkHelper sinkHelper ("ns3::UdpSocketFactory", sinkLocalAddress);
   
    apps.Add(sinkHelper.Install(dest));
    apps.Start(Seconds(1));
    apps.Stop(Seconds(SIM_TIME));

/*	Address sinkLocalAddress(InetSocketAddress (Ipv4Address::GetAny (), port));
	PacketSinkHelper sinkHelper ("ns3::UdpSocketFactory", sinkLocalAddress);
	app.Add(sinkHelper.Install(dest));*/
  OnOffHelper sendHelper ("ns3::UdpSocketFactory", Address ());
  sendHelper.SetAttribute("StartTime", TimeValue(MilliSeconds(0)));
  //random on time!
  int kbrate = (int)(rate*1000);
  char stringrate[50];
  sprintf (stringrate, "%dkb/s", kbrate);
  sendHelper.SetAttribute("DataRate", StringValue(stringrate));
  sendHelper.SetAttribute("OnTime", StringValue ("Constant:1.0"));
  sendHelper.SetAttribute("OffTime", StringValue ("Constant:0.0"));
  sendHelper.SetAttribute("Remote", AddressValue(InetSocketAddress(dest->GetObject<Ipv4>()->GetAddress(1,0).GetLocal(), port)));
  apps.Add(sendHelper.Install(source));

}

int main (int argc, char *argv[])
{
  std::string phyMode ("ErpOfdmRate54Mbps");
  std::string conMode ("ErpOfdmRate54Mbps");
  //std::string phyMode ("DsssRate1Mbps");
  //std::string conMode ("DsssRate1Mbps");
  ApplicationContainer apps;
  uint32_t packetSize = 1000; // bytes
  uint32_t numPackets = 0;
  double interval = 1.0; // seconds
  bool verbose = false;
 Ipv4StaticRoutingHelper routes;
 Ptr<Ipv4StaticRouting> current;
 
 FlowMonitorHelper flowmon; 
 Ptr<FlowMonitor> monitor;
 CommandLine cmd;

  cmd.AddValue ("phyMode", "Wifi Phy mode", phyMode);
  //cmd.AddValue ("rss", "received signal strength", rss);
  cmd.AddValue ("packetSize", "size of application packet sent", packetSize);
  cmd.AddValue ("numPackets", "number of packets generated", numPackets);
  cmd.AddValue ("interval", "interval (seconds) between packets", interval);
  cmd.AddValue ("verbose", "turn on all WifiNetDevice log components", verbose);

  cmd.Parse (argc, argv);
  // Convert to time object
  Time interPacketInterval = Seconds (interval);

  // disable fragmentation for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
  // turn off RTS/CTS for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));
  // Fix non-unicast data rate to be the same as that of unicast
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode", 
                      StringValue (phyMode));

  NodeContainer c;
  c.Create (2);

  // The below set of helpers will help us to put together the wifi NICs we want
  WifiHelper wifi;
  if (verbose)
    {
      wifi.EnableLogComponents ();  // Turn on all Wifi logging
    }
  wifi.SetStandard (WIFI_PHY_STANDARD_80211g);
  //wifi.SetStandard (WIFI_PHY_STANDARD_80211b);
  YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
  
  //This is the parameter to specify which antenna model we use
  //YansWifiPhyHelper will create ns3::IsotropicAntennaModel(the same as Omni-directional antenna) antenna by default
  //here we use directional antenna instend  
  wifiPhy.SetAntenna("ns3::CosineAntennaModel");


  // This is one parameter that matters when using FixedRssLossModel
  // set it to zero; otherwise, gain will be added
  //wifiPhy.Set ("RxGain", DoubleValue (0) ); 
  // ns-3 supports RadioTap and Prism tracing extensions for 802.11b
  wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO); 

  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  // The below FixedRssLossModel will cause the rss to be fixed regardless
  // of the distance between the two stations, and the transmit power
  wifiChannel.AddPropagationLoss ("ns3::LogDistancePropagationLossModel" , "ReferenceDistance",DoubleValue(120));
  
 
  wifiPhy.SetChannel (wifiChannel.Create ());

  // Add a non-QoS upper mac, and disable rate control
  NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode",StringValue (phyMode),
                                "ControlMode",StringValue (conMode));
  // Set it to adhoc mode
  wifiMac.SetType ("ns3::AdhocWifiMac");
  NetDeviceContainer devices = wifi.Install (wifiPhy, wifiMac, c);
  
  //add additional interfaces
  NetDeviceContainer tempdevices = wifi.Install (wifiPhy, wifiMac, c);  
  devices.Add(tempdevices);

  //print size of netdevice container
  printf("devices: %d\n",devices.GetN());


  //setting channels
  //Ptr<WifiNetDevice> currentWnDevice;
  
  //destination
  c.Get(0)->GetDevice(0)->GetObject<WifiNetDevice>()->GetPhy()->SetChannelNumber(1);
  c.Get(0)->GetDevice(1)->GetObject<WifiNetDevice>()->GetPhy()->SetChannelNumber(3);
  
  //source
  c.Get(1)->GetDevice(0)->GetObject<WifiNetDevice>()->GetPhy()->SetChannelNumber(1);
  c.Get(1)->GetDevice(1)->GetObject<WifiNetDevice>()->GetPhy()->SetChannelNumber(3);



  // Note that with FixedRssLossModel, the positions below are not 
  // used for received signal strength. 
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, 0.0, 0.0));
  positionAlloc->Add (Vector (100.0, 0.0, 0.0));
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (c);

  InternetStackHelper internet;
  internet.Install (c);

  Ipv4AddressHelper ipv4;
  NS_LOG_INFO ("Assign IP Addresses.");
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i = ipv4.Assign (devices);
  

  current = routes.GetStaticRouting(c.Get(1)->GetObject<Ipv4>());
   //we specify which antenna is going to be receiving here
  Ipv4Address destIP = c.Get(0)->GetObject<Ipv4>()->GetAddress(1,0).GetLocal();
  //we specify the sending antenna here
  current->AddHostRouteTo(destIP,destIP,1);
 
 //cout << "Number of interfaces on node 0: " << (int)c.Get(0)->GetObject<Ipv4>()->GetNInterfaces() << endl;
 TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> recvSink = Socket::CreateSocket (c.Get (0), tid);
  
  //InetSocketAddress local = InetSocketAddress (Ipv4Address("0.0.0.0"), 80);
  InetSocketAddress local = InetSocketAddress (destIP, 80);

  recvSink->Bind (local);
  recvSink->SetRecvCallback (MakeCallback (&ReceivePacket));

  Ptr<Socket> source = Socket::CreateSocket (c.Get (1), tid);
  //InetSocketAddress remote = InetSocketAddress (Ipv4Address ("255.255.255.255"), 80);
  InetSocketAddress remote = InetSocketAddress (destIP, 80);
  source->SetAllowBroadcast (true);
  source->Connect (remote);
  
  //udp ap from source to dest
  UDP_Setup (apps ,c.Get(1), c.Get(0), 10, 80);
  
//receiver one
Ptr<WifiNetDevice> currentWnDevice; 
currentWnDevice = c.Get(0)->GetDevice(0)->GetObject<WifiNetDevice>();
Ptr<CosineAntennaModel> currentAnt = currentWnDevice->GetPhy()->GetObject<YansWifiPhy>()->GetAntenna()->GetObject<CosineAntennaModel>();
currentAnt->SetBeamwidth(60);
currentAnt->SetOrientation(0);

//sender one
currentWnDevice = c.Get(1)->GetDevice(0)->GetObject<WifiNetDevice>();
currentAnt = currentWnDevice->GetPhy()->GetObject<YansWifiPhy>()->GetAntenna()->GetObject<CosineAntennaModel>();
currentAnt->SetBeamwidth(60);
currentAnt->SetOrientation(180);

//receive two
currentWnDevice = c.Get(0)->GetDevice(1)->GetObject<WifiNetDevice>();
currentAnt = currentWnDevice->GetPhy()->GetObject<YansWifiPhy>()->GetAntenna()->GetObject<CosineAntennaModel>();
currentAnt->SetBeamwidth(60);
currentAnt->SetOrientation(0);

//sender two
currentWnDevice = c.Get(1)->GetDevice(1)->GetObject<WifiNetDevice>();
currentAnt = currentWnDevice->GetPhy()->GetObject<YansWifiPhy>()->GetAntenna()->GetObject<CosineAntennaModel>();
currentAnt->SetBeamwidth(60);
currentAnt->SetOrientation(180);

  Simulator::Stop (Seconds (SIM_TIME));
  monitor = flowmon.Install(c);
  // Tracing
  wifiPhy.EnablePcap ("wifi-simple-adhoc", devices);

  // Output what we are doing
 // NS_LOG_UNCOND ("Testing " << numPackets  << " packets sent with receiver rss " << rss );

  Simulator::ScheduleWithContext (source->GetNode ()->GetId (),
                                  Seconds (1.0), &GenerateTraffic, 
                                  source, packetSize, numPackets, interPacketInterval);

  Simulator::Run ();

  monitor->CheckForLostPackets (); 
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ()); 
  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats (); 
  int counter = 0;
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i) 
   { 
      cout << "Flow               : " << counter++ << endl;
      cout << "Bytes transmitted  : " << (int)(i->second.txBytes) << endl;
      cout << "Bytes received     : " << (int)(i->second.rxBytes) << endl;
      cout << "Throughput         : " << (( (double)(i->second.rxBytes) * 8)/ 1000)/(SIM_TIME-1) << "kbps" << endl;
      cout << "Loss rate          : " << 1- ((double)(i->second.rxBytes)/(double)(i->second.txBytes)) << endl << endl;
          
   } 



  Simulator::Destroy ();

  return 0;
}


