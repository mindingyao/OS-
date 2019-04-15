from twilio.rest import Client
from datetime import datetime
import time

account ="AC723c61f1a20a9e24513414ce92a01033"
token ="0f92451d28d726daa88905ec78b97ae9"

def send_sms(to_mobile, text, tw_mobile='+12342064833'):
	client = Client(account, token)
	try:
		message = client.messages.create(to=to_mobile, from_=tw_mobile,
body=text)
		print('message status:',message.status)
		return True
	except Exception as e:
		print(e)
		return False

if __name__=='__main__':

	print('Script running!')
	while  True:
		now = datetime.now()
		#print('time:',now)
		if now.hour == 8 :
			gap = 25-now.day
			res = send_sms('+8615651810716','\n 温馨提示：距离开学还有'+str(gap)+'天。\
				\n 道路千万条，学习第一条。\n 寒假不学习，开学两行泪。')
			if res:
				print('Congratulation！Message send ok at:',datetime.strftime(now,'%Y-%m-%d %H:%M:%S'))
				time.sleep(60*60)
			else:
				print('Sorry,Message send failed!')

		time.sleep(5)