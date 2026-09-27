# How fast a walk cycle's planted foot moves relative to the hips, per key.
# In the game the body moves at the walk speed while the clip plays one key per game frame (60 keys/s).
# If the planted foot does not move back as fast as the body moves forward, the feet skate on the floor.
# A foot counts as planted while it is the lower of the two and within 2 cm of its lowest point.
import os
import maya.cmds as cmds
import maya.mel as mel

cmds.loadPlugin('fbxmaya', quiet=True)
NS = 'mixamorig:'
BASE_DIR = 'C:/git/god_syu/god/AZEFIGHTER/Assets/Model/knight/'


def sample(times):
    rows = []
    for t in times:
        cmds.currentTime(t)
        hips = cmds.xform(NS + 'Hips', q=True, ws=True, t=True)
        feet = {}
        for side in ('Left', 'Right'):
            p = cmds.xform(NS + side + 'Foot', q=True, ws=True, t=True)
            feet[side] = (p[2] - hips[2], p[1])
        rows.append(feet)
    return rows


for clip in os.environ['STRIDE_CLIPS'].split(','):
    cmds.file(new=True, force=True)
    mel.eval('FBXImport -f "%s%s.fbx"' % (BASE_DIR, clip))
    times = cmds.keyframe(NS + 'Hips.rotateX', q=True, timeChange=True) or []
    rows = sample(times)
    low = {s: min(r[s][1] for r in rows) for s in ('Left', 'Right')}
    speeds = []
    for a, b in zip(rows, rows[1:]):
        for s, o in (('Left', 'Right'), ('Right', 'Left')):
            planted = a[s][1] <= a[o][1] and a[s][1] < low[s] + 2.0 and b[s][1] < low[s] + 2.0
            if planted:
                speeds.append(b[s][0] - a[s][0])
    avg = sum(speeds) / len(speeds) if speeds else 0.0
    print('RESULT %-10s planted foot %.2f cm/key relative to hips (%d samples) -> %.0f cm/s at 60 keys/s' % (
        clip, avg, len(speeds), avg * 60.0))
