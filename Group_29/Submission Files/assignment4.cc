#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/drop-tail-queue.h"
#include <fstream>
#include <string>

using namespace ns3;


NS_LOG_COMPONENT_DEFINE ("CS 343: Lab 4");

class MyApp : public Application
{
	public:
		MyApp ();
		virtual ~MyApp();
		void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);
		void ChangeRate(DataRate newrate);

	private:
		virtual void StartApplication (void);
		virtual void StopApplication (void);
		void ScheduleTx (void);
		void SendPacket (void);
		Ptr<Socket> m_socket;
		Address m_peer;
		uint32_t m_packetSize;
		uint32_t m_nPackets;
		DataRate m_dataRate;
		EventId m_sendEvent;
		bool m_running;
		uint32_t m_packetsSent;
};

// Constructor
MyApp::MyApp ()
	: m_socket (0),
	m_peer (),
	m_packetSize (0),
	m_nPackets (0),
	m_dataRate (0),
	m_sendEvent (),
	m_running (false),
	m_packetsSent (0)
{
}

// Destructor
MyApp::~MyApp()
{
	m_socket = 0;
}

// Initializing member variables
void
MyApp::Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate)
{
	m_socket = socket;
	m_peer = address;
	m_packetSize = packetSize;
	m_nPackets = nPackets;
	m_dataRate = dataRate;
}

// Overridden implementation Application::StartApplication
void
MyApp::StartApplication (void)
{
	m_running = true;
	m_packetsSent = 0;
	m_socket->Bind ();
	m_socket->Connect (m_peer);
	SendPacket ();
}

// Stop creating simulation events
void
MyApp::StopApplication (void)
{
	m_running = false;
	if (m_sendEvent.IsRunning ())
	{
		Simulator::Cancel (m_sendEvent);
	}
	if (m_socket)
	{
		m_socket->Close ();
	}
}

// To start the chain of events that describes the Application behavior
void
MyApp::SendPacket (void)
{
	Ptr<Packet> packet = Create<Packet> (m_packetSize);
	m_socket->Send (packet);
	if (++m_packetsSent < m_nPackets)
	{
		ScheduleTx ();
	}
}

// To schedule another transmit event (a SendPacket) until the Application decides it has sent enough
void
MyApp::ScheduleTx (void)
{
	if (m_running)
	{
		Time tNext (Seconds (m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ())));
		m_sendEvent = Simulator::Schedule (tNext, &MyApp::SendPacket, this);
	}
}

// To update link rate
void
MyApp::ChangeRate(DataRate newrate)
{
   m_dataRate = newrate;
   return;
}

// Helper function to increase rate
void
IncRate (Ptr<MyApp> app, DataRate rate)
{
	app->ChangeRate(rate);
    return;
}


// Calculates throughput
void
calculateThroughput (FlowMonitorHelper *flowmon, Ptr<FlowMonitor> monitor, Ptr<OutputStreamWrapper> stream)
{
	std::cout << "\nt = " << Simulator::Now ().GetSeconds () << " sec\n";
	*stream->GetStream () << "\n\n";
	*stream->GetStream () << "Time = " << Simulator::Now ().GetSeconds () << " sec\n\n";
	*stream->GetStream () << "Flow ID\t\tProtocol\tSource\t\t\tDestination\t\tThroughPut (in Kbps)\n";
	monitor->CheckForLostPackets ();

	Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon->GetClassifier ());
	std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
	double req_sum = 0, req_sum_sq = 0;
	int n = 0;
	for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i) {
		Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
					
		int flowId = i->first;
		double throughput = i->second.rxBytes * 8.0 / ((i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds())*1024);
				
		std::string protocol;
		if(t.protocol == 6) {
			protocol = "TCP";
		}
		else {
			protocol = "UDP";
		}
				
		req_sum += throughput;
		req_sum_sq += throughput * throughput ;
		n++;
				
		*stream->GetStream () <<  std::to_string(flowId) << "\t\t\t" << protocol << "\t\t\t" << t.sourceAddress <<"\t\t"<< t.destinationAddress << "\t\t" << std::to_string(throughput) << "\n";
		
	}
	
	double FairnessIndex = (req_sum * req_sum)/ (6 * req_sum_sq) ;
	*stream->GetStream () <<  "\nFairnessIndex:	" << std::to_string(FairnessIndex) << "\n";
	
	Simulator::Schedule (Seconds(5.0), &calculateThroughput, flowmon, monitor, stream);
	
}


int
main (int argc, char *argv[])
{
	// Set the time resolution to one nanosecond (default value)
	Time::SetResolution (Time::NS);
	TypeId tid = TypeId::LookupByName ("ns3::TcpNewReno");
	Config::Set ("/NodeList/*/$ns3::TcpL4Protocol/SocketType", TypeIdValue (tid));

	
	for(int numPackets = 10; numPackets <= 800; numPackets *= 2) {
		
		int bufferSize = numPackets*1536;
		
		// Generating trace files in ASCII Format
  		AsciiTraceHelper asciiTrace;
		std::string traceFileName = "TraceFile_" + std::to_string((bufferSize/1024)) + "KB.txt";
		
		// Creating Output stream
		Ptr<OutputStreamWrapper> stream;
  		stream = asciiTrace.CreateFileStream (traceFileName);
		*stream->GetStream () << "Buffer Size \t= " << (bufferSize/1024.0) << " KB\n";
    		
		
		// Create 8 nodes =  6 hosts + 2 routers
		NodeContainer nodes;
		nodes.Create (8);
			
		
		// Create 100 Mbps, 10 ms link (Host to Router)
		PointToPointHelper pointToPointChannel1;
		pointToPointChannel1.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
		pointToPointChannel1.SetChannelAttribute ("Delay", StringValue ("10ms"));
		
		
		// Create 10 Mbps, 100 ms link (Router to Router)
		PointToPointHelper pointToPointChannel2;
		pointToPointChannel2.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
		pointToPointChannel2.SetChannelAttribute ("Delay", StringValue ("100ms"));
		pointToPointChannel2.SetQueue("ns3::DropTailQueue<Packet>", "MaxSize", QueueSizeValue (QueueSize("85p")));  

		
		// Create node container
		NodeContainer host1_router1 = NodeContainer (nodes.Get (0), nodes.Get (3));
		NodeContainer host2_router1 = NodeContainer (nodes.Get (1), nodes.Get (3)); 
		NodeContainer host3_router1 = NodeContainer (nodes.Get (2), nodes.Get (3));
		NodeContainer router1_router2 = NodeContainer (nodes.Get (3), nodes.Get (4));
		NodeContainer router2_host4 = NodeContainer (nodes.Get (4), nodes.Get (5));
		NodeContainer router2_host5 = NodeContainer (nodes.Get (4), nodes.Get (6));
		NodeContainer router2_host6 = NodeContainer (nodes.Get (4), nodes.Get (7));

		
		// Creating network devices on tje container
		NetDeviceContainer device_host1_router1 = pointToPointChannel1.Install (host1_router1);
		NetDeviceContainer device_host2_router1 = pointToPointChannel1.Install (host2_router1);
		NetDeviceContainer device_host3_router1 = pointToPointChannel1.Install (host3_router1);
		NetDeviceContainer device_router1_router2 = pointToPointChannel2.Install (router1_router2);
		NetDeviceContainer device_router2_host4 = pointToPointChannel1.Install (router2_host4);
		NetDeviceContainer device_router2_host5 = pointToPointChannel1.Install (router2_host5);
		NetDeviceContainer device_router2_host6 = pointToPointChannel1.Install (router2_host6);

		
		// Installing protocol stack on the nodes
		InternetStackHelper stack;
		stack.Install (nodes);
		
		
		// Associate the devices on our nodes with IP addresses
		Ipv4AddressHelper address;
		address.SetBase ("10.1.1.0", "255.255.255.0");
		Ipv4InterfaceContainer interface_host1_router1 = address.Assign (device_host1_router1);
		
		address.SetBase ("10.1.2.0", "255.255.255.0");
		Ipv4InterfaceContainer interface_host2_router1 = address.Assign (device_host2_router1);

		address.SetBase ("10.1.3.0", "255.255.255.0");
		Ipv4InterfaceContainer interface_host3_router1 = address.Assign (device_host3_router1);

		address.SetBase ("10.1.4.0", "255.255.255.0");
		Ipv4InterfaceContainer interface_router1_router2 = address.Assign (device_router1_router2);

		address.SetBase ("10.1.5.0", "255.255.255.0");
		Ipv4InterfaceContainer interface_router2_host4 = address.Assign (device_router2_host4);

		address.SetBase ("10.1.6.0", "255.255.255.0");
		Ipv4InterfaceContainer interface_router2_host5 = address.Assign (device_router2_host5);

		address.SetBase ("10.1.7.0", "255.255.255.0");
		Ipv4InterfaceContainer interface_router2_host6 = address.Assign (device_router2_host6);

		
		// Turn on global static routing
		Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

		
		
		// ============================================================================================
		// TCP connection 1 => between H1 & H4
		
		uint16_t sinkPort_H1_H4 = 8080;
		Address sinkAddress_tcp1 (InetSocketAddress(interface_router2_host4.GetAddress (1), sinkPort_H1_H4));
		PacketSinkHelper packetSinkHelper_tcp1 ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort_H1_H4));
		ApplicationContainer sinkApps_tcp1 = packetSinkHelper_tcp1.Install (nodes.Get (5));
		sinkApps_tcp1.Start (Seconds (0.));
		sinkApps_tcp1.Stop (Seconds (75.));

		Ptr<Socket> ns3TcpSocket_tcp1 = Socket::CreateSocket (nodes.Get (0), TcpSocketFactory::GetTypeId ());
		ns3TcpSocket_tcp1->SetAttribute("SndBufSize",  ns3::UintegerValue(bufferSize));
		ns3TcpSocket_tcp1->SetAttribute("RcvBufSize",  ns3::UintegerValue(bufferSize));


		Ptr<MyApp> app_tcp1 = CreateObject<MyApp> ();
		app_tcp1->Setup (ns3TcpSocket_tcp1, sinkAddress_tcp1, 1536, 100000, DataRate ("20Mbps"));
		nodes.Get (0)->AddApplication (app_tcp1);
		app_tcp1->SetStartTime (Seconds (1.));
		app_tcp1->SetStopTime (Seconds (75.));
		
		
		
		// ============================================================================================
		// TCP connection 2 => between H2 & H5

		uint16_t sinkPort_H2_H5 = 8081;
		Address sinkAddress_tcp2 (InetSocketAddress(interface_router2_host5.GetAddress (1), sinkPort_H2_H5));
		PacketSinkHelper packetSinkHelper_tcp2 ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort_H2_H5));
		ApplicationContainer sinkApps_tcp2 = packetSinkHelper_tcp2.Install (nodes.Get (6));
		sinkApps_tcp2.Start (Seconds (0.));
		sinkApps_tcp2.Stop (Seconds (75.));

		Ptr<Socket> ns3TcpSocket_tcp2 = Socket::CreateSocket (nodes.Get (1), TcpSocketFactory::GetTypeId ());
		ns3TcpSocket_tcp2->SetAttribute("SndBufSize",  ns3::UintegerValue(bufferSize));
		ns3TcpSocket_tcp2->SetAttribute("RcvBufSize",  ns3::UintegerValue(bufferSize));

		Ptr<MyApp> app_tcp2 = CreateObject<MyApp> ();
		app_tcp2->Setup (ns3TcpSocket_tcp2, sinkAddress_tcp2, 1536, 100000, DataRate ("20Mbps"));
		nodes.Get (1)->AddApplication (app_tcp2);
		app_tcp2->SetStartTime (Seconds (1.));
		app_tcp2->SetStopTime (Seconds (75.));


		
		// ============================================================================================
		// TCP connection 3 => between H3 & H6

		uint16_t sinkPort_H3_H6 = 8082;
		Address sinkAddress_tcp3 (InetSocketAddress(interface_router2_host6.GetAddress (1), sinkPort_H3_H6));
		PacketSinkHelper packetSinkHelper_tcp3 ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort_H3_H6));
		ApplicationContainer sinkApps_tcp3 = packetSinkHelper_tcp3.Install (nodes.Get (7));
		sinkApps_tcp3.Start (Seconds (0.));
		sinkApps_tcp3.Stop (Seconds (75.));

		Ptr<Socket> ns3TcpSocket_tcp3 = Socket::CreateSocket (nodes.Get (2), TcpSocketFactory::GetTypeId ());
		ns3TcpSocket_tcp3->SetAttribute("SndBufSize",  ns3::UintegerValue(bufferSize));
		ns3TcpSocket_tcp3->SetAttribute("RcvBufSize",  ns3::UintegerValue(bufferSize));

		Ptr<MyApp> app_tcp3 = CreateObject<MyApp> ();
		app_tcp3->Setup (ns3TcpSocket_tcp3, sinkAddress_tcp3, 1536, 100000, DataRate ("20Mbps"));
		nodes.Get (2)->AddApplication (app_tcp3);
		app_tcp3->SetStartTime (Seconds (1.));
		app_tcp3->SetStopTime (Seconds (75.));



		// ============================================================================================
		// TCP connection 4 => between H1 & H2

		uint16_t sinkPort_H1_H2 = 8083;
		Address sinkAddress_tcp4 (InetSocketAddress(interface_host2_router1.GetAddress (0), sinkPort_H1_H2));
		PacketSinkHelper packetSinkHelper_tcp4 ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort_H1_H2));
		ApplicationContainer sinkApps_tcp4 = packetSinkHelper_tcp4.Install (nodes.Get (1));
		sinkApps_tcp4.Start (Seconds (0.));
		sinkApps_tcp4.Stop (Seconds (75.));

		Ptr<Socket> ns3TcpSocket_tcp4 = Socket::CreateSocket (nodes.Get (0), TcpSocketFactory::GetTypeId ());
		ns3TcpSocket_tcp4->SetAttribute("SndBufSize",  ns3::UintegerValue(bufferSize));
		ns3TcpSocket_tcp4->SetAttribute("RcvBufSize",  ns3::UintegerValue(bufferSize));

		Ptr<MyApp> app_tcp4 = CreateObject<MyApp> ();
		app_tcp4->Setup (ns3TcpSocket_tcp4, sinkAddress_tcp4, 1536, 100000, DataRate ("20Mbps"));
		nodes.Get (0)->AddApplication (app_tcp4);
		app_tcp4->SetStartTime (Seconds (1.));
		app_tcp4->SetStopTime (Seconds (75.));
			

		
		// ============================================================================================
		// UDP connection 1 => between H2 & H6

		uint16_t sinkPort_H2_H6 = 8084;
		Address sinkAddress_udp1 (InetSocketAddress (interface_router2_host6.GetAddress (1), sinkPort_H2_H6));
		PacketSinkHelper packetSinkHelper_udp1 ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort_H2_H6));
		ApplicationContainer sinkApps_udp1 = packetSinkHelper_udp1.Install (nodes.Get (7));
		sinkApps_udp1.Start (Seconds (30.));
		sinkApps_udp1.Stop (Seconds (75.));

		Ptr<Socket> ns3UdpSocket_udp1 = Socket::CreateSocket (nodes.Get (1), UdpSocketFactory::GetTypeId ());
		ns3UdpSocket_udp1->SetAttribute("RcvBufSize",  ns3::UintegerValue(bufferSize));

		Ptr<MyApp> app_udp1 = CreateObject<MyApp> ();
		app_udp1->Setup (ns3UdpSocket_udp1, sinkAddress_udp1, 1536, 100000, DataRate ("20Mbps"));
		nodes.Get (1)->AddApplication (app_udp1);
		app_udp1->SetStartTime (Seconds (31.));
		app_udp1->SetStopTime (Seconds (75.));
			
		
		
		// ============================================================================================
		// UDP connection 2 => between H3 & H4
		
		uint16_t sinkPort_H3_H4 = 8085;
		Address sinkAddress_udp2 (InetSocketAddress (interface_router2_host4.GetAddress (1), sinkPort_H3_H4));
		PacketSinkHelper packetSinkHelper_udp2 ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort_H3_H4));
		ApplicationContainer sinkApps_udp2 = packetSinkHelper_udp2.Install (nodes.Get (5));
		sinkApps_udp2.Start (Seconds (30.));
		sinkApps_udp2.Stop (Seconds (75.));

		Ptr<Socket> ns3UdpSocket_udp2 = Socket::CreateSocket (nodes.Get (2), UdpSocketFactory::GetTypeId ());
		ns3UdpSocket_udp2->SetAttribute("RcvBufSize",  ns3::UintegerValue(bufferSize));

		Ptr<MyApp> app_udp2 = CreateObject<MyApp> ();
		app_udp2->Setup (ns3UdpSocket_udp2, sinkAddress_udp2, 1536, 100000, DataRate ("20Mbps"));
		nodes.Get (2)->AddApplication (app_udp2);
		app_udp2->SetStartTime (Seconds (31.));
		app_udp2->SetStopTime (Seconds (75.));
			
		Ptr<FlowMonitor> monitor;
		FlowMonitorHelper flowmon;
		
		monitor = flowmon.InstallAll();
		
		Simulator::Schedule (Seconds(5.0), &calculateThroughput, &flowmon, monitor, stream);
		Simulator::Schedule (Seconds(35.0), &IncRate, app_udp1, DataRate("30Mbps"));
		Simulator::Schedule (Seconds(40.0), &IncRate, app_udp1, DataRate("40Mbps"));
		Simulator::Schedule (Seconds(45.0), &IncRate, app_udp1, DataRate("50Mbps"));
		Simulator::Schedule (Seconds(50.0), &IncRate, app_udp1, DataRate("60Mbps"));
		Simulator::Schedule (Seconds(55.0), &IncRate, app_udp1, DataRate("70Mbps"));
		Simulator::Schedule (Seconds(60.0), &IncRate, app_udp1, DataRate("80Mbps"));
		Simulator::Schedule (Seconds(65.0), &IncRate, app_udp1, DataRate("90Mbps"));
		Simulator::Schedule (Seconds(70.0), &IncRate, app_udp1, DataRate("100Mbps"));
		
		
		NS_LOG_INFO ("Run Simulation");
		Simulator::Stop (Seconds(76.0));
		Simulator::Run ();
		
		Simulator::Destroy ();
		
		if(numPackets == 640)
			numPackets = 400;
	}
	
	return 0;	
}
	
	
	