// 1. Place the sourcecode.cc file in the scratch folder.
// 2. Open a terminal in the ns3 directory and run the following commands one by one:
//    ./waf --run "sourcecode --TCPvariant=TcpNewReno"
//    ./waf --run "sourcecode --TCPvariant=TcpHybla"
//    ./waf --run "sourcecode --TCPvariant=TcpWestwood"
//    ./waf --run "sourcecode --TCPvariant=TcpScalable"
//    ./waf --run "sourcecode --TCPvariant=TcpVegas"
//    Note: Incase the above commands do not work by copying and pasting them on the terminal,
//          type them mannually and then run them
// 3. Files containing congestion window size data, dropped packets data and transferred bytes data 
//    will be created in the ns3 folder, for every TCP variant. Use python (or any other graphing tool) 
//    to obtain the desired plots.

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/stats-module.h"
#include <fstream>
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TCP_variants_comparison");

class temp : public Application
{
  public:
    temp();
    virtual ~temp();

    Ptr<Socket>     tempsocket;
    Address         tempaddress;
    uint32_t        temppacketSize;
    uint32_t        tempnPackets;
    DataRate        tempdataRate;
    EventId         tempsendEvent;
    bool            temprunning;
    uint32_t        temppacketsSent;

    static TypeId returnid(void)
    {
      static TypeId id = TypeId("temp").SetParent<Application>().SetGroupName("Tutorial").AddConstructor<temp>();
      return id;
    }

    void initialize(Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate)
    {
      temppacketSize = packetSize;
      tempaddress = address;
      tempdataRate = dataRate;
      tempnPackets = nPackets;
      tempsocket = socket;
    }

  private:
    virtual void StartApplication(void)
    {
      temppacketsSent = 0;
      temprunning = true;

      // Decides binding for Ipv4 or Ipv6
      if(!InetSocketAddress::IsMatchingType(tempaddress))
        tempsocket->Bind6();

      else
        tempsocket->Bind();

      tempsocket->Connect(tempaddress);
      send();
    }

    virtual void StopApplication(void)
    {
      temprunning = false;

      if(tempsendEvent.IsRunning())
        Simulator::Cancel(tempsendEvent);

      if(tempsocket)
        tempsocket->Close();
    }

    void next(void)
    {
      if(temprunning)
      {
        Time tNext(Seconds(temppacketSize * 8 / static_cast<double>(tempdataRate.GetBitRate())));
        tempsendEvent = Simulator::Schedule(tNext, &temp::send, this);
      }
    }

    void send(void)
    {
      Ptr<Packet> packet = Create<Packet>(temppacketSize);
      tempsocket->Send(packet);

      if(++temppacketsSent < tempnPackets)
        next();
    }
};

// Overriding the member variables of Application class
temp::temp(): tempsocket(0),tempaddress(), temppacketSize(0), tempnPackets(0), tempdataRate(0), tempsendEvent(), temprunning(false), temppacketsSent(0)
{
}

// Socket is deleted after the connection is closed
temp::~temp()
{
  tempsocket = 0;
}

// Create Flow Monitor
Ptr<FlowMonitor> fmon;
FlowMonitorHelper fhelp;

// Vector to store number of dropped packets as a function of time
std::vector<std::pair<float,int>> vecdrop;

// Number of dropped packets
void dropped()
{
  std::map<FlowId, FlowMonitor::FlowStats> stats = fmon->GetFlowStats();
  float time = Simulator::Now().GetSeconds();
  int count = 0;

  if(stats[1].packetsDropped.size() >= 5)
    count += stats[1].packetsDropped[3] + stats[1].packetsDropped[4];

  if(stats[2].packetsDropped.size() >= 5)
    count += stats[2].packetsDropped[3] + stats[2].packetsDropped[4];

  vecdrop.push_back({time, count});
  Simulator::Schedule(Seconds(0.01), &dropped);
}

// Congestion window data
static void congestionwindow(Ptr<OutputStreamWrapper> z, uint32_t x, uint32_t y)
{
  *z->GetStream() << Simulator::Now().GetSeconds() << "\t" << y << "\n";
}

int main(int argc, char *argv[])
{
  // Input TCP variant from the command line
  std::string TCPvariant = "TcpWestwood";
  CommandLine cmd;
  cmd.AddValue ("TCPvariant", "Transport protocol to use: TcpNewReno, "
                  "TcpHybla, TcpVegas, TcpScalable, "
                  " TcpWestwood ", TCPvariant);
  cmd.Parse(argc, argv);

  // Compare the TCPvariant string with known values and set the TCP type accordingly
  if(TCPvariant.compare("TcpNewReno") == 0)
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpNewReno::GetTypeId()));

  else if(TCPvariant.compare("TcpHybla") == 0)
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpHybla::GetTypeId()));

  else if(TCPvariant.compare("TcpWestwood") == 0)
  {
  // the default protocol type in ns3::TcpWestwood is WESTWOOD
  // for WESTWOODPLUS, add Config::SetDefault("ns3::TcpWestwood::ProtocolType", EnumValue(TcpWestwood::WESTWOODPLUS));
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpWestwood::GetTypeId()));
    Config::SetDefault("ns3::TcpWestwood::FilterType", EnumValue(TcpWestwood::TUSTIN));
  }

  else if(TCPvariant.compare("TcpScalable") == 0)
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpScalable::GetTypeId()));

  else if(TCPvariant.compare("TcpVegas") == 0)
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpVegas::GetTypeId()));

  else
  {
    NS_LOG_DEBUG("Allowed TCP variants: TCP New Reno, TCP Hybla, TCP Westwood, TCP Scalable, TCP Vegas");
    exit(1);
  }

  // Create N0 and N1
  NodeContainer networknodes;
  networknodes.Create(2);

  // Create the link and set the desired link attributes
  PointToPointHelper link;
  link.SetDeviceAttribute("DataRate", StringValue("1Mbps"));
  link.SetChannelAttribute("Delay", StringValue("10ms"));
  link.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("1500B"));
  // link.SetQueue("ns3::DropTailQueue");

  // Connect the created link between N0 and N1
  NetDeviceContainer connectdevices;
  connectdevices = link.Install(networknodes);

  // Install the internet stack
  InternetStackHelper internetstack;
  internetstack.Install(networknodes);

  // Setup IP addresses
  Ipv4AddressHelper ipv4address;
  ipv4address.SetBase("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer ipv4if = ipv4address.Assign(connectdevices);

  // Create the TCP receiver with port = 4200, simulate it from t = 0 to t = 1800 (ms)
  PacketSinkHelper receivetcp("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 4200));
  ApplicationContainer applicationsink = receivetcp.Install(networknodes.Get(1));  
  applicationsink.Start(Seconds(0));
  applicationsink.Stop(Seconds(1.8));

  // Create the TCP socket and set the source as N0
  Ptr<Socket> socket = Socket::CreateSocket(networknodes.Get(0), TcpSocketFactory::GetTypeId());

  // Create an object of type temp (defined in the beginning) for the FTP connection and install it on N0
  Ptr<temp> ftp = CreateObject<temp>();
  ftp->initialize(socket, InetSocketAddress(ipv4if.GetAddress(1), 4200), 512, 100000, DataRate("1Mbps"));

  networknodes.Get(0)->AddApplication(ftp);
  ftp->SetStartTime(Seconds(0));
  ftp->SetStopTime(Seconds(1.8));

  // CBR start and end times as given
  float start[5] = {0.2, 0.4, 0.6, 0.8, 1};
  float end[5] = {1.8, 1.8, 1.2, 1.4, 1.6};

  for(int i=0; i<5; i++)
  {
    // Simulate CBR traffic using OnOffHelper
    OnOffHelper cbrsim("ns3::UdpSocketFactory", InetSocketAddress(ipv4if.GetAddress(1), 6900));
    cbrsim.SetAttribute("OnTime",  StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    cbrsim.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));

    // Set data rate, start time and end time
    cbrsim.SetAttribute("DataRate", StringValue("300Kbps"));
    cbrsim.SetAttribute("StartTime", TimeValue(Seconds(start[i])));
    cbrsim.SetAttribute("StopTime", TimeValue(Seconds(end[i])));
    
    // Create an app to send CBR traffic from N0
    ApplicationContainer cbr;
    cbr.Add(cbrsim.Install(networknodes.Get(0)));

    // Start CBR traffic
    cbr.Start(Seconds(start[i]));
    cbr.Stop(Seconds(end[i]));
  }

  // Create a sink to receive data
  PacketSinkHelper sinkudp("ns3::UdpSocketFactory",InetSocketAddress(Ipv4Address::GetAny(), 6900));

  // Receive the UDP traffic in N1
  ApplicationContainer sinkcbr = sinkudp.Install(networknodes.Get(1));
  sinkcbr.Start(Seconds(0));
  sinkcbr.Stop(Seconds(1.8));

  // Store congestion window data
  AsciiTraceHelper ascii;
  Ptr<OutputStreamWrapper> z = ascii.CreateFileStream("congestion_" + TCPvariant);
  socket->TraceConnectWithoutContext("CongestionWindow", MakeBoundCallback(&congestionwindow, z));

  // Use FileHelper to write out the packet byte count over time
  FileHelper writef;

  // Store transferred bytes data
  writef.ConfigureFile("sentbytes_" + TCPvariant, FileAggregator::FORMATTED);
  writef.Set2dFormat("%.3e\t%.0f");
  writef.WriteProbe("ns3::Ipv4PacketProbe", "/NodeList/*/$ns3::Ipv4L3Protocol/Tx", "OutputBytes");

  // Flow monitor
  fmon = fhelp.InstallAll();

  // Run simulation
  Simulator::Schedule(Seconds(0.01), &dropped);
  Simulator::Stop(Seconds(1.8));
  Simulator::Run();

  // Get the stats from Flow Monitor and write it to a file
  std::map<FlowId, FlowMonitor::FlowStats> stats = fmon->GetFlowStats();
  fmon->SerializeToXmlFile("flowmonitor_" + TCPvariant, true, true);

  Simulator::Destroy();
  NS_LOG_INFO("Completed");

  // Store dropped packets data
  std::ofstream fout;
  fout.open("drop_" + TCPvariant);

  for(int i=0; i<vecdrop.size(); i++)
    fout << vecdrop[i].first << " " << vecdrop[i].second << "\n";

  return 0;
}