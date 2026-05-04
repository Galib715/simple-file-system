from flask import Flask, jsonify, request, send_from_directory
import subprocess
import json
import os

app = Flask(__name__)

SFS_PATH = '/mnt/c/Users/User/simple-file-system/sfs'
GUI_DIR   = os.path.dirname(os.path.abspath(__file__))

os.chdir('/mnt/c/Users/User/simple-file-system')

def run_sfs(*args):
    cmd = [SFS_PATH] + list(args)
    result = subprocess.run(cmd, capture_output=True, text=True,
                            cwd='/mnt/c/Users/User/simple-file-system')
    return result.stdout.strip(), result.stderr.strip()

def parse_json_output(stdout):
    lines = stdout.split('\n')
    json_start = next((i for i, l in enumerate(lines) if l.strip().startswith('{')), None)
    if json_start is None:
        return None
    return '\n'.join(lines[json_start:])

@app.route('/')
def index():
    return send_from_directory(GUI_DIR, 'index.html')

@app.route('/api/state')
def get_state():
    stdout, stderr = run_sfs('json')
    json_str = parse_json_output(stdout)
    if not json_str:
        return jsonify({'error': 'Could not read disk state', 'details': stderr}), 500
    try:
        data = json.loads(json_str)
        return jsonify(data)
    except Exception as e:
        return jsonify({'error': 'JSON parse failed', 'details': str(e)}), 500

@app.route('/api/format', methods=['POST'])
def format_disk():
    stdout, _ = run_sfs('format')
    return jsonify({'message': stdout})

@app.route('/api/ls')
def ls():
    stdout, _ = run_sfs('ls')
    return jsonify({'output': stdout})

@app.route('/api/mkdir', methods=['POST'])
def mkdir():
    name = request.json.get('name', '')
    if not name:
        return jsonify({'error': 'Name required'}), 400
    stdout, _ = run_sfs('mkdir', name)
    return jsonify({'message': stdout})

@app.route('/api/create', methods=['POST'])
def create():
    name = request.json.get('name', '')
    if not name:
        return jsonify({'error': 'Name required'}), 400
    stdout, _ = run_sfs('create', name)
    return jsonify({'message': stdout})

@app.route('/api/write', methods=['POST'])
def write():
    name    = request.json.get('name', '')
    content = request.json.get('content', '')
    if not name or not content:
        return jsonify({'error': 'Name and content required'}), 400
    stdout, _ = run_sfs('write', name, content)
    return jsonify({'message': stdout})

@app.route('/api/read')
def read():
    name = request.args.get('name', '')
    if not name:
        return jsonify({'error': 'Name required'}), 400
    stdout, _ = run_sfs('read', name)
    return jsonify({'content': stdout})

@app.route('/api/delete', methods=['POST'])
def delete():
    name = request.json.get('name', '')
    if not name:
        return jsonify({'error': 'Name required'}), 400
    stdout, _ = run_sfs('delete', name)
    return jsonify({'message': stdout})

if __name__ == '__main__':
    print("SFS GUI running at http://localhost:5000")
    app.run(host='0.0.0.0', port=5000, debug=False, use_reloader=False)
