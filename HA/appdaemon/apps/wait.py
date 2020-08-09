import datetime, time 

def wait_available(self, entity, is_domain):
	count = 0
	dev = []
	if(isinstance(entity, str)):
		dev.append(entity)
	else:
		dev = entity
	while(count < 10):
		ret = True
		for i in dev:
			if(is_domain):
				#self.log(self.get_state(i))
				if(not(self.get_state(i) == None)):
					break
			else:
				if(not(self.entity_exists(i))):
					self.log("dev "+str(i)+" not ready")
					ret = False
					#self.terminate()
					break
		if(ret):
			self.log("All devices online after "+str(count)+" sec")
			return True
		else:
			count += 1
			time.sleep(1)
	self.log("entities never came online ")
	self.log(dev)
	return False
