import os
import BaseHTTPServer
import SimpleHTTPServer

def main():
  print '''
Started HTTP server.  http://localhost/... maps to the current directory:
  %s
Press Ctrl-Break or Ctrl-C to exit.
''' % os.getcwd()

  server_class = BaseHTTPServer.HTTPServer
  handler_class = SimpleHTTPServer.SimpleHTTPRequestHandler

  server_addr = ('', 80)
  httpd = server_class(server_addr, handler_class)
  httpd.serve_forever()

if __name__ == '__main__':
  main()
