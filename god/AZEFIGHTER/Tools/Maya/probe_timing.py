# For each clip: how many keys it has, the scene frame rate it was authored at, and how far the hips travel.
# The engine plays one key per game frame (60 per second), so a clip keyed at 30fps plays twice as fast
# as it was captured, and a clip slowed below speed 1.0 holds each key for several game frames.
import os
import maya.cmds as cmds
import maya.mel as mel

cmds.loadPlugin('fbxmaya', quiet=True)
NS = 'mixamorig:'
BASE_DIR = 'C:/git/god_syu/god/AZEFIGHTER/Assets/Model/knight/'
for clip in os.environ['TIMING_CLIPS'].split(','):
    cmds.file(new=True, force=True)
    mel.eval('FBXImport -f "%s%s.fbx"' % (BASE_DIR, clip))
    times = cmds.keyframe(NS + 'Hips.rotateX', q=True, timeChange=True) or []
    unit = cmds.currentUnit(q=True, time=True)
    print('RESULT %-18s keys %4d  first %.1f last %.1f  unit %s' % (clip, len(times), times[0] if times else -1, times[-1] if times else -1, unit))
