# Report how far forward (and how high) the striking hand / foot gets in each clip, in cm and in game units.
import os
import maya.cmds as cmds
import maya.mel as mel

cmds.loadPlugin('fbxmaya', quiet=True)
NS = 'mixamorig:'
SCALE = 0.014  # model scale used by Player::Load
BASE_DIR = 'C:/git/god_syu/god/AZEFIGHTER/Assets/Model/knight/'
for item in os.environ['REACH_CLIPS'].split(','):
    name, joint = item.split(':')
    cmds.file(new=True, force=True)
    mel.eval('FBXImport -f "%s%s.fbx"' % (BASE_DIR, name))
    times = set()
    for c in cmds.ls(type='animCurve'):
        times.update(cmds.keyframe(c, q=True, timeChange=True) or [])
    best = None
    for t in sorted(times):
        cmds.currentTime(t)
        p = cmds.xform(NS + joint, q=True, ws=True, t=True)
        if best is None or p[2] > best[1][2]:
            best = (t, p)
    t, p = best
    print('RESULT %-18s %-10s max forward at f=%2d  z=%5.1fcm y=%5.1fcm  -> x=%.2f y=%.2f units' % (name, joint, t, p[2], p[1], p[2] * SCALE, p[1] * SCALE))
