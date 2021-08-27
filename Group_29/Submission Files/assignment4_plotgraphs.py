import matplotlib.pyplot as plt
from pprint import pprint
import rglob


def parseFiles():
  # (bufsz, protocol, src, dest, flow_id, time, throughput)
  res = []

  # (bufsz, time, fairness)
  res2 = []

  for file_name in rglob.rglob(".", "*.txt"):
      with open(file_name) as fp:
          lines = fp.readlines()
          bufsz = int(lines[0].split()[-2])
          lines = lines[3:]

          while len(lines) > 0:
              while(len(lines[0].split()) == 0):
                  lines = lines[1:]

              time = int(lines[0].split()[-2])
              lines = lines[3:]

              if time >= 35:
                limit = 10
              else:
                limit = 8

              for i in range(limit):
                  vals = lines[i].split()
                  flowid = int(vals[0])
                  protocol = vals[1]
                  src = vals[2]
                  dest = vals[3]
                  throughput = float(vals[4])
                  res.append((bufsz, protocol, src, dest, flowid, time, throughput))

              lines = lines[limit + 1:]
              fairness = float(lines[0].split()[-1])
              res2.append((bufsz, time, fairness))
              lines = lines[3:]
    
  return res, res2


def plotsForThroughput(res):
  bufsizes = sorted(set(map(lambda x: x[0], res)))
  flowids = sorted(set(map(lambda x: x[4], res)))

  for flowid in flowids:
      for bufsz in bufsizes:
          pts = list(map(lambda x: (x[-2], x[-1]) , filter(lambda x: x[-3] == flowid and x[0] == bufsz, res)))
          protocol = "TCP"
          if flowid > 8:
            protocol = "UDP"

          plt.title(f'Flow {flowid} - Transport Layer Protocol = {protocol}')
          plt.xlabel('Time (in sec)')
          plt.ylabel('Throughput (in Kbps)')
          # print(list(map(lambda x: x[0], pts)))
          # print(list(map(lambda x: x[1], pts)))
          # print()
          plt.plot(list(map(lambda x: x[0], pts)), list(map(lambda x: x[1], pts)), label="Buffer %dKB"%(bufsz))

      plt.legend(loc='lower right')
      plt.savefig(f'Flow{flowid}.png')
      plt.show()


def plotsForFairshare(res2):
  maxtime = 75
  pts = list(map(lambda x: (x[0], x[-1]), filter(lambda x: x[-2] == maxtime, res2)))
  x = list(map(lambda x: x[0], pts))
  y = list(map(lambda x: x[1], pts))
  d = dict(zip(x, y))
  combined = sorted(d.items())

  x, y = zip(*combined)
  plt.title('Impact of buffer size on the fair share of bandwidth')
  plt.xlabel('Buffer Size (in KB)')
  plt.ylabel('Fairness Index')
  plt.plot(x, y)
  plt.show()

  bufsizes = sorted(list(set(map(lambda x: x[0], res2))))
  # print(bufsizes)
  plt.clf()
  for bufsz in bufsizes:
      pts = list(map(lambda x: (x[1], x[2]), filter(lambda x: x[0] == bufsz, res2)))
      # print(pts)
      plt.plot(list(map(lambda x: x[0], pts)), list(map(lambda x: x[1], pts)), label="Buffer %dKB"%(bufsz,))
  
  plt.legend(loc='upper right')
  plt.title('Impact of buffer size on the fair share of bandwidth as simulation progresses')
  plt.xlabel('Time (in sec)')
  plt.ylabel('Fairness Index')
  plt.show()


def main():
  res, res2 = parseFiles()
  plotsForThroughput(res)
  plotsForFairshare(res2)
  

if __name__=="__main__":
    main()