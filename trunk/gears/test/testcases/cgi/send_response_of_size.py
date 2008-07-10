size = int(self.query['size'][0])

self.send_response(200)
self.send_header('Content-Type', 'text/html')
self.send_header('Content-Length', size)
self.end_headers()

self.outgoing.append('a'*size)
