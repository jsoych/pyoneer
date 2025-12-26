import os
import json
import socket
import unittest

# Define global constants
WORKER = os.getenv('PYONEER_SOCKET_PATH')
BUFFSIZE = 1024

# Get worker and job status codes
worker_states = ['not_assigned', 'working', 'not_working']
job_states = [None, 'not_ready', 'ready', 'running', 'completed', 'incomplete']

class SmallTest(unittest.TestCase):
    ''' Tests the run_job command. '''

    def setUp(self):
        # Connect to the worker
        self.sock = socket.socket(socket.AF_UNIX)
        self.sock.connect(WORKER)

    def tearDown(self):
        self.sock.close()

    def send_commnad(self, command, buffsize=BUFFSIZE):
        ''' Sends the command to the worker and returns its response. '''
        obj = json.dumps(command)
        self.sock.send(obj.encode())
        data = self.sock.recv(buffsize)
        return json.loads(data)

    def test_get_status(self):
        res = self.send_commnad({'command': 'get_status'})
        self.assertIn(res['pyoneer'], worker_states)
    
    def test_assign_job(self):
        res = self.send_commnad({'command': 'assign',
        'job': {
            'id': 1,
            'tasks': [
                {'name': 'task.py'},
                {'name': 'task.py'}
            ]
        }})
        self.assertIn(res['pyoneer'], worker_states)
    
    def test_run_job(self):
        res = self.send_commnad({'command': 'run', 
        'job': {
            'id': 1,
            'tasks': [
                {'name': 'task.py'},
                {'name': 'task.py'}
            ]
        }})
        self.assertIn(res['pyoneer'], worker_states)

    def test_stop_worker(self):
        res = self.send_commnad({'command': 'stop'})
        self.assertIn(res['pyoneer'], worker_states)
    
    def test_start_worker(self):
        res = self.send_commnad({'command': 'start'})
        self.assertIn(res['pyoneer'], worker_states)

if __name__ == '__main__':
    unittest.main()