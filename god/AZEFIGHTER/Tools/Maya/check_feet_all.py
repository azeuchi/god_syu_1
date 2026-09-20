# For each clip, report how far each foot moves from its frame-0 position (planted feet should stay ~0).
import os
import maya.cmds as cmds
import maya.mel as mel

cmds.loadPlugin('fbxmaya', quiet=True)
NS = 'mixamorig:'
BASE_DIR = 'C:/git/god_syu/god/AZEFIGHTER/Assets/Model/knight/'
for name in os.environ['CHECK_CLIPS'].split(','):
    cmds.file(new=True, force=True)
    mel.eval('FBXImport -f "%s%s.fbx"' % (BASE_DIR, name))
    times = set()
    for c in cmds.ls(type='animCurve'):
        times.update(cmds.keyframe(c, q=True, timeChange=True) or [])
    drift = {}
    lowest = {}
    for side in ('Left', 'Right'):
        cmds.currentTime(0)
        p0 = cmds.xform(NS + side + 'Foot', q=True, ws=True, t=True)
        worst = 0.0
        low = 1e9
        for t in sorted(times):
            cmds.currentTime(t)
            p = cmds.xform(NS + side + 'Foot', q=True, ws=True, t=True)
            worst = max(worst, sum((a - b) ** 2 for a, b in zip(p, p0)) ** 0.5)
            low = min(low, p[1])
        drift[side] = worst
        lowest[side] = low
    print('RESULT %-18s L move max %5.1f cm (lowest y %5.1f)   R move max %5.1f cm (lowest y %5.1f)' % (
        name, drift['Left'], lowest['Left'], drift['Right'], lowest['Right']))
