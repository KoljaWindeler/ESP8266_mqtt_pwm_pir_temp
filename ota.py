#! python3.4
###demo code provided by Steve Cope at www.steves-internet-guide.com
##email steve@steves-internet-guide.com
###Free to use for any purpose
"""
Send File Using MQTT
"""
import time, os
import paho.mqtt.client as paho
import hashlib
broker="192.168.2.84"
filename=".pioenvs/esp01_1m/firmware.bin" #file to send
#filename="../releases/20180310.bin" #file to send
topic="dev97/s/ota"
#topic="s/ota"
qos=1
data_block_size=900
fo=open(filename,"rb")
#fout=open("1out.txt","wb") #use a different filename
# for outfile as I'm rnning sender and receiver together
###demo code provided by Steve Cope at www.steves-internet-guide.com
##email steve@steves-internet-guide.com
###Free to use for any purpose
def process_message(msg):
   """ This is the main receiver code
   """
   if len(msg)==200: #is header or end
      msg_in=msg.decode("utf-8")
      msg_in=msg_in.split(",,")
      if msg_in[0]=="end": #is it really last packet?
         in_hash_final=in_hash_md5.hexdigest()
         if in_hash_final==msg_in[2]:
            print("File copied OK - valid hash  ",in_hash_final)
         else:
            print("Bad file receive   ",in_hash_final)
         return False
      else:
         if msg_in[0]!="header":
            in_hash_md5.update(msg)
            return True
         else:
            return False
   else:
      in_hash_md5.update(msg)
      return True

#define callback
def on_message(client, userdata, message):
   time.sleep(1)
   #print("received message =",str(message.payload.decode("utf-8")))
   #if process_message(message.payload):
    #  fout.write(message.payload)

def on_publish(client, userdata, mid):
    #logging.debug("pub ack "+ str(mid))
    client.mid_value=mid
    client.puback_flag=True

## waitfor loop
def wait_for(client,msgType,period=1,wait_time=2000,running_loop=False):
    client.running_loop=running_loop #if using external loop
    wcount=0
    while True:
        #print("waiting"+ msgType)
        if msgType=="PUBACK":
            if client.on_publish:
                if client.puback_flag:
                    return True

        if not client.running_loop:
            client.loop(.01)  #check for messages manually
        time.sleep(period)
        #print("loop flag ",client.running_loop)
        wcount+=1
        if wcount>wait_time:
            print("return from wait loop taken too long")
            return False
    return True

def send_header(filename):
   size=os.path.getsize(filename)
   header="A"+str(size)
   header=bytearray(header,"utf-8")
   #header.extend(b','*(200-len(header)))
   print(header)
   c_publish(client,topic,header,qos)

def send_end(filename,md5):
   end="C"
   end=bytearray(end,"utf-8")+md5.encode("utf-8")
   #end.extend(b','*(200-len(end)))
   print(end)
   c_publish(client,topic,end,qos)

def c_publish(client,topic,out_message,qos):
   res,mid=client.publish(topic,out_message,qos) #publish
   if res==0: #published ok
      if wait_for(client,"PUBACK",running_loop=True):
         if mid==client.mid_value:
            print("match mid ",str(mid))
            client.puback_flag=False #reset flag
         else:
            raise SystemExit("not got correct puback mid so quitting")

      else:
         raise SystemExit("not got puback so quitting")


client= paho.Client("client-001")  #create client object client1.on_publish = on_publish                          #assign function to callback client1.connect(broker,port)                                 #establish connection client1.publish("data/files","on")
######
client.on_message=on_message
client.on_publish=on_publish
client.puback_flag=False #use flag in publish ack
client.mid_value=None
#####
print("connecting to broker ",broker)
client.username_pw_set("ha","ah")
client.connect(broker)#connect
client.loop_start() #start loop to process received messages

print("subscribing ")
client.subscribe(topic)#subscribe
time.sleep(2)
start=time.time()

print("publishing ")
send_header(filename)
Run_flag=True
count=0
out_hash_md5 = hashlib.md5()
in_hash_md5 = hashlib.md5()

while Run_flag:
   chunk=fo.read(data_block_size)
   if chunk:
      out_hash_md5.update(chunk) #update hash
      out_message="B".encode("utf-8")+bytes([count//256])+bytes([count%256])
      out_message=out_message+chunk
      #print(" length =",type(out_message))
      c_publish(client,topic,out_message,qos)
      count = count + 1
   else:
      #end of file so send hash
      out_message=out_hash_md5.hexdigest()
      send_end(filename,out_message)
      #print("out Message ",out_message)
      #res,mid=client.publish("data/files",out_message,qos=1)#publish
      Run_flag=False
time_taken=time.time()-start
print("took ",time_taken)
time.sleep(4)
client.disconnect() #disconnect
client.loop_stop() #stop loop
#fout.close()
fo.close()
