from http.server import BaseHTTPRequestHandler, HTTPServer
import json

class RequestHandler(BaseHTTPRequestHandler):
    def do_POST(self):
        if self.path == '/data':
            content_length = int(self.headers['Content-Length'])  # Get the size of the data
            post_data = self.rfile.read(content_length)  # Read the data
            try:
                # Attempt to decode the JSON data
                data = json.loads(post_data)
                print("Received data:", data)
                
                # Send a response back to the client
                self.send_response(200)
                self.send_header('Content-type', 'application/text')
                self.end_headers()
                self.wfile.write("OK".encode('utf-8'))
            except json.JSONDecodeError:
                # Handle JSON decoding error
                self.send_response(400)
                self.send_header('Content-type', 'application/text')
                self.end_headers()
                self.wfile.write("Invalid JSON".encode('utf-8'))
        else:
            self.send_response(404)
            self.end_headers()

def run(server_class=HTTPServer, handler_class=RequestHandler, port=80):
    server_address = ('', port)
    httpd = server_class(server_address, handler_class)
    print(f'Starting server on port {port}...')
    httpd.serve_forever()

if __name__ == "__main__":
    run()
