# Compare a clip's last frame with the pose it returns to (CrouchIdle / Idle): how far each hand and
# elbow is off and how much each fist is turned. A big difference pops when the next clip starts.
import math
import os
import maya.cmds as cmds
import maya.mel as mel
import maya.api.OpenMaya as om

cmds.loadPlugin('fbxmaya', quiet=True)
NS = 'mixamorig:'
BASE_DIR = 'C:/git/god_syu/god/AZEFIGHTER/Assets/Model/knight/'


def sample(clip, frame):
    cmds.file(new=True, force=True)
    mel.eval('FBXImport -f "%s%s.fbx"' % (BASE_DIR, clip))
    cmds.currentTime(frame)
    chest_inv = om.MMatrix(cmds.xform(NS + 'Spine2', q=True, ws=True, matrix=True)).inverse()
    out = {}
    for side in ('Right', 'Left'):
        for j in ('ForeArm', 'Hand'):
            p = om.MPoint(*cmds.xform(NS + side + j, q=True, ws=True, t=True)) * chest_inv
            out[side + j] = om.MVector(p.x, p.y, p.z)
        m = om.MMatrix(cmds.xform(NS + side + 'Hand', q=True, ws=True, matrix=True))
        out[side + 'Z'] = (om.MVector(m[8], m[9], m[10]) * chest_inv).normal()
    return out


for item in os.environ['LOOP_CLIPS'].split(','):
    clip, rest = item.split(':')
    cmds.file(new=True, force=True)
    mel.eval('FBXImport -f "%s%s.fbx"' % (BASE_DIR, clip))
    last = max(cmds.keyframe(NS + 'Hips.rotateX', q=True, timeChange=True))
    a, b = sample(clip, last), sample(rest, 0)
    for side in ('Right', 'Left'):
        print('RESULT %-18s %-5s elbow off %4.1fcm  hand off %4.1fcm  fist turned %5.1f deg' % (
            clip, side, (a[side + 'ForeArm'] - b[side + 'ForeArm']).length(), (a[side + 'Hand'] - b[side + 'Hand']).length(),
            math.degrees(math.acos(max(-1.0, min(1.0, a[side + 'Z'] * b[side + 'Z']))))))
