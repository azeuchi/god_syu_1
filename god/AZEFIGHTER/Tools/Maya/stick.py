# Render stick-figure contact sheets of FBX animations (side view Z-Y, front view X-Y).
# Pure Python PNG writer so it runs inside mayabatch without extra modules.
import struct, zlib, math, os
import maya.cmds as cmds
import maya.mel as mel

cmds.loadPlugin('fbxmaya', quiet=True)
BASE = os.environ.get('STICK_BASE', r'C:/git/god_syu/god/AZEFIGHTER/Assets/Model/knight/')
OUT = os.environ.get('STICK_OUT', os.path.join(os.path.dirname(os.path.abspath(__file__)) if '__file__' in globals() else '.', 'preview'))
if not os.path.isdir(OUT):
    os.makedirs(OUT)

class Canvas(object):
    def __init__(self, w, h):
        self.w, self.h = w, h
        self.px = bytearray([255, 255, 255] * (w * h))
    def dot(self, x, y, c, r=1):
        for dy in range(-r, r + 1):
            for dx in range(-r, r + 1):
                xx, yy = int(x + dx), int(y + dy)
                if 0 <= xx < self.w and 0 <= yy < self.h:
                    i = (yy * self.w + xx) * 3
                    self.px[i:i + 3] = bytes(c)
    def line(self, x0, y0, x1, y1, c, r=1):
        n = int(max(abs(x1 - x0), abs(y1 - y0))) + 1
        for k in range(n + 1):
            t = k / float(n)
            self.dot(x0 + (x1 - x0) * t, y0 + (y1 - y0) * t, c, r)
    def save(self, path):
        raw = b''.join(b'\x00' + bytes(self.px[y * self.w * 3:(y + 1) * self.w * 3]) for y in range(self.h))
        def chunk(tag, data):
            return struct.pack('>I', len(data)) + tag + data + struct.pack('>I', zlib.crc32(tag + data) & 0xffffffff)
        png = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', self.w, self.h, 8, 2, 0, 0, 0))
        png += chunk(b'IDAT', zlib.compress(raw, 6)) + chunk(b'IEND', b'')
        with open(path, 'wb') as f:
            f.write(png)

def color_of(j):
    n = j.split(':')[-1]
    if n.startswith('Left'):
        return (40, 90, 230)
    if n.startswith('Right'):
        return (220, 40, 40)
    return (60, 60, 60)

def skip(j):
    n = j.split(':')[-1]
    return any(s in n for s in ('Index', 'Middle', 'Pinky', 'Ring', 'Thumb', 'Eye', '_End'))

def load(name):
    cmds.file(new=True, force=True)
    mel.eval('FBXImport -f "%s%s.fbx"' % (BASE, name))

def frames_of():
    curves = cmds.ls(type='animCurve')
    ts = set()
    for c in curves:
        ts.update(cmds.keyframe(c, q=True, timeChange=True) or [])
    return min(ts), max(ts)

def sample_pose(joints, t):
    cmds.currentTime(t)
    return dict((j, cmds.xform(j, q=True, ws=True, t=True)) for j in joints)

def sheet(name, count=10, times=None, label=None):
    joints = [j for j in cmds.ls(type='joint') if not skip(j)]
    parents = {}
    for j in joints:
        p = cmds.listRelatives(j, parent=True, type='joint')
        if p and not skip(p[0]):
            parents[j] = p[0]
    if times is None:
        a, b = frames_of()
        times = [a + (b - a) * i / float(count - 1) for i in range(count)]
    cell_w, cell_h = 150, 230
    scale = 1.05  # px per cm
    views = ((lambda v: v[2]), (lambda v: v[0]), (lambda v: v[2] * 0.707 - v[0] * 0.707))  # side / front / 3-4
    canvas = Canvas(cell_w * len(times), cell_h * len(views))
    for i, t in enumerate(times):
        pose = sample_pose(joints, t)
        for row, horiz in enumerate(views):
            ox = cell_w * i + cell_w * 0.5
            oy = cell_h * (row + 1) - 12
            # ground line and frame separator
            canvas.line(cell_w * i + 4, oy, cell_w * (i + 1) - 4, oy, (200, 200, 200), 0)
            canvas.line(cell_w * i, cell_h * row, cell_w * i, cell_h * (row + 1), (230, 230, 230), 0)
            def P(v):
                return ox + horiz(v) * scale, oy - v[1] * scale
            for j, p in parents.items():
                x0, y0 = P(pose[p]); x1, y1 = P(pose[j])
                canvas.line(x0, y0, x1, y1, color_of(j), 1)
            for j in joints:
                if j.endswith('Hand') or j.endswith('Head'):
                    x, y = P(pose[j]); canvas.dot(x, y, color_of(j), 3)
        # frame index tick marks at top: one dot per 10 frames
        for k in range(int(t) // 10):
            canvas.dot(cell_w * i + 8 + k * 6, 6, (0, 150, 0), 2)
    path = os.path.join(OUT, '%s.png' % (label or name))
    canvas.save(path)
    print('SHEET', path, ['%.0f' % t for t in times])

TARGETS = os.environ.get('STICK_TARGETS', 'LightPunch,HeavyPunch,Jump,Hadouken').split(',')
TIMES = [float(t) for t in os.environ['STICK_TIMES'].split(',')] if os.environ.get('STICK_TIMES') else None
for n in TARGETS:
    load(n)
    sheet(n, times=TIMES)
