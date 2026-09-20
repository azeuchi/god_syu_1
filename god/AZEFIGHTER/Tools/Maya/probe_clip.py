# Print world positions of the hips, right shoulder / elbow / hand for some frames of a generated clip.
import os
import maya.cmds as cmds
import maya.mel as mel

cmds.loadPlugin('fbxmaya', quiet=True)
NS = 'mixamorig:'
clip = os.environ['PROBE_CLIP']
cmds.file(new=True, force=True)
mel.eval('FBXImport -f "C:/git/god_syu/god/AZEFIGHTER/Assets/Model/knight/%s.fbx"' % clip)
for f in [float(v) for v in os.environ['PROBE_FRAMES'].split(',')]:
    cmds.currentTime(f)
    row = []
    for j in ('Hips', 'RightArm', 'RightForeArm', 'RightHand', 'Head'):
        p = cmds.xform(NS + j, q=True, ws=True, t=True)
        row.append('%s=(%.0f,%.0f,%.0f)' % (j, p[0], p[1], p[2]))
    print('RESULT f=%d %s' % (f, '  '.join(row)))
