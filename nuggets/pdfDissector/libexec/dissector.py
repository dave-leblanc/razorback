#!/opt/jython/bin/jython -J-Xmx2048m

import sys
import os
import SocketServer

class MyTCPHandler(SocketServer.StreamRequestHandler):
	
	def write(self, text):
		return self.request.send(text)
	
	def handle(self):
		try:
			sys.stdout = self
			(cmd, pdf_file) = self.rfile.readline().strip().split(" ")

			if cmd == "DIE":
				print "DIE"
				os._exit(0)
			if cmd != "FILE":
				return
			
			data = None
			decodedData = None
			pdf_data = open(pdf_file, "r").read()

			# Start by parsing the pdf
			pdf = PdfFileHelpers.parse(pdf_file)

			# Run all of our file modules
			for m in getModules('file'):
				m.VulnerabilityCheck(pdf, pdf_data).check()

			# Process each object
			for obj in pdf.objects:
				
				# Check for javascript
				if obj.stream:
				  data = convertToUchar(obj.stream.data)

				  if obj.stream.decodedData:
					 decodedData = convertToUchar(obj.stream.decodedData)

					 if len(obj.stream.decodedData) > 0:
						result = getJavaScript(pdf, obj, zynamics_path)
						
						# This should be javascript
						if result:
							for m in getModules('javascript'):
							  m.VulnerabilityCheck(pdf, obj, result, data, decodedData).check()

							# No reason to process it further
							continue

				# Check the non-javascript modules
				for m in getModules('object'):
				  m.VulnerabilityCheck(pdf, obj, data, decodedData).check()
		except:
			pass
		
if __name__ == "__main__":

	# Update this to point to your installed pdf-dissector directory
	zynamics_path = sys.argv[1]

	sys.path.append(zynamics_path + '/dissector.jar')
	sys.path.append(zynamics_path + '/jide-editor.jar')

	from com.zynamics.pdf.api import PdfFileHelpers
	from modules.utils import *

	HOST, PORT = "localhost", 8888

	# Create the server, binding to localhost on port 9999
	server = SocketServer.TCPServer((HOST, PORT), MyTCPHandler)
	server.allow_reuse_address = True

	# Activate the server; this will keep running until you
	# interrupt the program with Ctrl-C
	server.serve_forever()
