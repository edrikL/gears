size = int(self.query['size'][0])

self.send_response(200)
self.send_header('Content-Type', 'text/plain')
self.send_header('Content-Length', size)
self.end_headers()

for i in xrange(0, size):
  self.outgoing.append('a')
else:
  self.outgoing.append (None)
