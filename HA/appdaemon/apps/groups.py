###########################################################################################  *
#                                                                                         #
#  Rene Tode ( hass@reot.org )                                                            #
#  2017/11/29 Germany                                                                     #
#                                                                                         #
#  wildcard groups                                                                        #
#                                                                                         #
#  arguments:                                                                             #
#  name: your_name                                                                        #
#  device_type: sensor # or any devicetype                                                #
#  entity_part: "any_part"                                                                #
#  entities: # list of entities                                                           #
#    - sensor.any_entity                                                                  #
#  hidden: False # or True                                                                #
#  view: True # or False                                                                  #
#  assumed_state: False # or True                                                         #
#  friendly_name: Your Friendly Name                                                      #
#  nested_view: True # or False                                                           #
#                                                                                         #
###########################################################################################

import appdaemon.plugins.hass.hassapi as hass

class create_group(hass.Hass):

  def initialize(self):
    domains = ["switch","light","sensor"]
    domains = ["switch","light"]
    ap_ssid = ["IOT255","IOT0","IOT1","IOT254"]
    entitylist = []
    for d in domains:
      entitylist.append([])
      for a in ap_ssid:
        entitylist[domains.index(d)].append([])
      entitylist[domains.index(d)].append([])
#    print(*entitylist)

    for d in domains:
#      print("check domain "+d)
      all_entities = self.get_state(d)
      for entity in all_entities:
#        print("check "+entity)
        if "dev" in entity:
          p=entity.split(".")
          if(len(p)>=2):
            dev = p[1].split("_")[0]
            ssid = "sensor."+str(dev)+"_ssid"
            if(self.entity_exists(ssid)):
              ssid = self.get_state(ssid)
              if(ssid=="IOT254"):
                ssid="IOT255"
#              print("has _ssid "+ssid)
              if(ssid in ap_ssid):
                 a = ap_ssid.index(ssid)
              else:
#                print("---> ssid not found")
                 a=0
#              print("add to entitylist["+str(domains.index(d))+"]["+str(a)+"] "+entity)

              entitylist[domains.index(d)][a].append(entity)
#        else:
#           print("not a device -> "+entity)
#        entitylist.append(entity.lower())
    hidden = "False"
    view = "True"
    assumed_state = "False"
    friendly_name = "Test"


#    print(*entitylist)


    for d in domains:
      for a in ap_ssid:
        a_i = ap_ssid.index(a)
        if(a_i<0):
          a_i=len(ap_ssid)
        group = "group."+a+"_"+d
#        print("adding to "+group+" all entities from entitylist["+str(domains.index(d))+"]["+str(a_i)+"]")
        self.set_state(group.lower(),state="on",attributes={"view": view,"hidden": hidden,"assumed_state": assumed_state,"friendly_name": friendly_name,"entity_id": entitylist[domains.index(d)][a_i]})
