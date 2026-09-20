# Print where the feet are on each frame of a generated clip, to catch planted feet that slide.
import os
import maya.cmds as cmds
import maya.mel as mel

cmds.loadPlugin('fbxmaya', quiet=True)
NS = 'mixamorig:'
path = os.environ['CHECK_FBX'].replace('\\', '/')
cmds.file(new=True, force=True)
mel.eval('FBXImport -f "%s"' % path)
curves = cmds.ls(type='animCurve')
times = set()
for c in curves:
    times.update(cmds.keyframe(c, q=True, timeChange=True) or [])
for t in sorted(times):
    cmds.currentTime(t)
    lf = cmds.xform(NS + 'LeftFoot', q=True, ws=True, t=True)
    rf = cmds.xform(NS + 'RightFoot', q=True, ws=True, t=True)
    print('RESULT f=%2d  L=(%6.1f %6.1f %6.1f)  R=(%6.1f %6.1f %6.1f)' % ((t,) + tuple(lf) + tuple(rf)))
