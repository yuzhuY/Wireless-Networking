/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
 /*
Yuzhu Yan, TU Delft, Embedded Systems, Stu. Num: 4468023
yuzhuyan@yahoo.com 
https://github.com/yuzhuY/Wireless-Networking
Based on third.cc 
*/

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h" 
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/ipv4-global-routing-helper.h" 
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-module.h" 
#include "ns3/random-variable-stream.h" 
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

using namespace ns3; 
NS_LOG_COMPONENT_DEFINE ("MyWifi");

int 
main (int argc, char *argv[]) 
{
	double StartTime = 0.0;
	double StopTime = 10.0;
	int nNodes = 5; /*number of node*/
	uint32_t payloadSize = 896 /*payload 1472*/;
	uint32_t maxPacket = 10000 /*10000*/ ; 
	StringValue DataRate;
	DataRate = StringValue("DsssRate11Mbps");/*1,2,5_5,11*/
	std::string rtsCts("900");

		// Create randomness based on time
		time_t timex;
		time(&timex); 
		RngSeedManager::SetSeed(timex); 
		RngSeedManager::SetRun(1);

		CommandLine cmd; 
		cmd.Parse (argc,argv);
		cmd.AddValue("rtsCts", "RTS/CTS threshold", rtsCts);
		Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold",
			StringValue(rtsCts));
		//test
		Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold",
			StringValue ("2200"));
		Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",
			StringValue (DataRate));

		// Create access point
		NodeContainer wifiApNode; 
		wifiApNode.Create (1);
		std::cout << "Access point created.." << '\n';

		// Create nodes
		NodeContainer wifiStaNodes; 
		wifiStaNodes.Create (nNodes); 
		std::cout << "Nodes created.." << '\n';

		YansWifiPhyHelper phy = YansWifiPhyHelper::Default (); 
		phy.Set ("RxGain", DoubleValue (0) );

		YansWifiChannelHelper channel;
		channel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel"); 
		channel.AddPropagationLoss ("ns3::FriisPropagationLossModel"); 
		phy.SetChannel (channel.Create ());

		WifiHelper wifi = WifiHelper::Default ();
		wifi.SetStandard (WIFI_PHY_STANDARD_80211b);
		std::cout << "Wifi 802.11b Phy & Channel configured.." << '\n';

		// configure MAC parameter
		wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager","DataMode", DataRate, "ControlMode", DataRate);


		// wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

		NqosWifiMacHelper mac = NqosWifiMacHelper::Default (); 
		std::cout << "Control rate configured.." << '\n';

		// configure SSID
		Ssid ssid = Ssid ("myWifi");

		mac.SetType ("ns3::StaWifiMac",
			"Ssid", SsidValue (ssid), 
			"ActiveProbing", BooleanValue (false));
		
		NetDeviceContainer staDevices;
		staDevices = wifi.Install (phy, mac, wifiStaNodes);

		mac.SetType ("ns3::ApWifiMac", 
			"Ssid", SsidValue (ssid));

		NetDeviceContainer apDevice;
		apDevice = wifi.Install (phy, mac, wifiApNode);
		std::cout << "SSID, ApDevice & StaDevice configured.." << '\n';
		// Configure nodes mobility 
		MobilityHelper mobility, mobilityAp;
		// Random Walk 2D Node Mobility Model 
		/*mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
			"Bounds", RectangleValue (Rectangle (-1000, 1000, -1000, 1000)), 
			"Distance", ns3::DoubleValue (300.0));*/
		//sta node mobility
		mobility.SetPositionAllocator("ns3::RandomDiscPositionAllocator",
			"Rho", StringValue("ns3::UniformRandomVariable[Min=40.0|Max=60.0]")
			);
		mobility.Install (wifiStaNodes);
		// Constant Mobility for Access Point, fix AP at (0,0)
		// Constant Mobility for Access Point
		mobilityAp.SetMobilityModel ("ns3::ConstantPositionMobilityModel"); 
		mobilityAp.Install (wifiApNode);
		std::cout << "Node mobility configured.." << '\n';

		// Internet stack 
		InternetStackHelper stack; 
		stack.Install (wifiApNode); 
		stack.Install (wifiStaNodes);

		// Configure IPv4 address
		Ipv4AddressHelper address;
		Ipv4Address addr;
		address.SetBase ("10.1.1.0", "255.255.255.0"); 
		Ipv4InterfaceContainer staNodesInterface; 
		Ipv4InterfaceContainer apNodeInterface; 
		staNodesInterface = address.Assign (staDevices); 
		apNodeInterface = address.Assign (apDevice);

		for(int i = 0 ; i < nNodes; i++) 
		{
			addr = staNodesInterface.GetAddress(i);
			std::cout << " Node " << i+1 << "\t "<< "IP Address "<<addr << std::endl; 
		}
		addr = apNodeInterface.GetAddress(0);
		std::cout << "Internet Stack & IPv4 address configured.." << '\n';
		
		// Create traffic generator (UDP)
		ApplicationContainer serverApp;
		UdpServerHelper myServer (4001); //port 4001
		serverApp = myServer.Install (wifiStaNodes.Get (0));
		serverApp.Start (Seconds(StartTime));
		serverApp.Stop (Seconds(StopTime));
		UdpClientHelper myClient (apNodeInterface.GetAddress (0), 4001); //port 4001 
		myClient.SetAttribute ("MaxPackets", UintegerValue (maxPacket)); 
		myClient.SetAttribute ("Interval", TimeValue (Time ("0.002"))); //packets/s 
		myClient.SetAttribute ("PacketSize", UintegerValue (payloadSize)); 
		ApplicationContainer clientApp = myClient.Install (wifiStaNodes.Get (0)); 
		clientApp.Start (Seconds(StartTime));
		clientApp.Stop (Seconds(StopTime+5)); 
		std::cout << "UDP traffic generated.." << '\n';

		// Calculate Throughput & Delay using Flowmonitor 
		FlowMonitorHelper flowmon;
		Ptr<FlowMonitor> monitor = flowmon.InstallAll();

		Simulator::Stop (Seconds(StopTime+2)); 
		Simulator::Run ();

		monitor->CheckForLostPackets ();
		//!!!!test
		int psent=0;
		int preceived=0;
		Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ()); 
		std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
		for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i) 
		{
			Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
			std::cout << "Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
			std::cout << " Tx Bytes: " << i->second.txBytes << "\n";
			std::cout << " Rx Bytes: " << i->second.rxBytes << "\n";
			std::cout << " Average Throughput: " << i->second.rxBytes * 8.0 / (i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds())/1024/nNodes << " kbps\n";
			std::cout << " Delay : " << i->second.delaySum / i->second.rxPackets << "\n";
			psent=psent+i->second.txBytes;
			preceived=preceived+i->second.rxBytes;
			std::cout <<"loss:"<<psent-preceived<<"\n";
			std::cout <<"send:"<<psent<<"\n";
			std::cout <<"received"<<preceived<<"\n";

		}
		Simulator::Destroy (); 
	return 0;
	}