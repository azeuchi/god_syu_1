# Print world positions of the main joints at frame 0 of a clip (default: CrouchIdle),
# used as reference numbers when authoring key poses.
import os
import maya.cmds as cmds
import maya.mel as mel

cmds.loadPlugin('fbxmaya', quiet=True)
NS = 'mixamorig:'
clip = os.environ.get('PROBE_CLIP', 'CrouchIdle')
cmds.file(new=True, force=True)
mel.eval('FBXImport -f "C:/git/god_syu/god/AZEFIGHTER/Assets/Model/knight/%s.fbx" ' % clip)
cmds.currentTime(0)
for j in ('Hips', 'Spine2', 'Head', 'LeftArm', 'RightArm', 'LeftHand', 'RightHand',
          'LeftUpLeg', 'RightUpLeg', 'LeftLeg', 'RightLeg', 'LeftFoot', 'RightFoot', 'LeftToeBase', 'RightToeBase'):
    print('RESULT %s %-12s %s' % (clip, j, [round(v, 1) for v in cmds.xform(NS + j, q=True, ws=True, t=True)]))
print('RESULT %s hips world rot %s' % (clip, [round(v, 1) for v in cmds.xform(NS + 'Hips', q=True, ws=True, ro=True)]))
curves = cmds.ls(type='animCurve')
times = set()
for c in curves:
    times.update(cmds.keyframe(c, q=True, timeChange=True) or [])
print('RESULT %s frames %s' % (clip, (min(times), max(times)) if times else None))
