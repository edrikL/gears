size = int(self.query['size'][0])
slowly = self.query.has_key('slowly')

self.send_response(200)
self.send_header('Content-Type', 'text/html')
self.send_header('Content-Length', size)
self.end_headers()

if slowly:
  time.sleep(0.5)
  
for i in xrange(0, size):
  if slowly:
    time.sleep(0.01);
  self.outgoing.append('a')
else:
  self.outgoing.append (None)
