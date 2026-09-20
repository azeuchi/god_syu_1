# Render the real knight mesh with a generated clip, from the same side angle as the game camera,
# so poses are judged on the actual model instead of stick figures.
#   RENDER_CLIP   : clip name in Assets/Model/knight (e.g. CrouchHeavyPunch)
#   RENDER_FRAMES : comma separated frames
import os
import maya.cmds as cmds
import maya.mel as mel

cmds.loadPlugin('fbxmaya', quiet=True)
BASE_DIR = 'C:/git/god_syu/god/AZEFIGHTER/Assets/Model/knight/'
HERE = os.path.dirname(os.path.abspath(__file__)) if '__file__' in globals() else '.'
OUT = os.path.join(HERE, 'preview', 'render').replace('\\', '/')
if not os.path.isdir(OUT):
    os.makedirs(OUT)

clip = os.environ['RENDER_CLIP']
frames = [float(f) for f in os.environ['RENDER_FRAMES'].split(',')]

cmds.file(new=True, force=True)
cmds.currentUnit(time='ntscf')
mel.eval('FBXImport -f "%sIdle.fbx"' % BASE_DIR)
# the clip only has the skeleton; merge its animation onto the skinned Idle rig
cmds.cutKey(cmds.ls(type='joint'), clear=True)
mel.eval('FBXImportMode -v exmerge')
mel.eval('FBXImport -f "%s%s.fbx"' % (BASE_DIR, clip))

# side view like the game: the character faces +Z, the camera looks at its right side
cam, cam_shape = cmds.camera(name='sideCam')
cmds.setAttr(cam + '.translate', -380.0, 85.0, 25.0)
cmds.setAttr(cam + '.rotate', -2.0, -90.0, 0.0)
cmds.setAttr(cam_shape + '.focalLength', 55.0)
for shape in cmds.ls(type='camera'):
    cmds.setAttr(shape + '.renderable', shape == cam_shape)

for m in cmds.ls(type='mesh'):
    sg = cmds.listConnections(m, type='shadingEngine') or []
    if not sg:
        cmds.sets(m, edit=True, forceElement='initialShadingGroup')

# plain grey material and soft lights so the silhouette and the joints read clearly
mat = cmds.shadingNode('lambert', asShader=True, name='poseGrey')
cmds.setAttr(mat + '.color', 0.75, 0.75, 0.78, type='double3')
sg = cmds.sets(renderable=True, noSurfaceShader=True, empty=True, name='poseGreySG')
cmds.connectAttr(mat + '.outColor', sg + '.surfaceShader', force=True)
for m in cmds.ls(type='mesh'):
    cmds.sets(m, edit=True, forceElement=sg)
cmds.ambientLight(intensity=0.35)
key = cmds.directionalLight(intensity=1.1)
cmds.setAttr(cmds.listRelatives(key, parent=True)[0] + '.rotate', -30.0, -60.0, 0.0)
cmds.setAttr('defaultRenderGlobals.enableDefaultLight', 0)

cmds.setAttr('defaultRenderGlobals.imageFormat', 32)  # png
cmds.setAttr('defaultResolution.width', 360)
cmds.setAttr('defaultResolution.height', 420)
cmds.setAttr('defaultResolution.deviceAspectRatio', 360.0 / 420.0)
cmds.workspace(fileRule=['images', OUT])

for f in frames:
    cmds.currentTime(f)
    path = cmds.render(cam, x=360, y=420)
    dst = os.path.join(OUT, '%s_%02d.png' % (clip, int(f))).replace('\\', '/')
    if os.path.exists(dst):
        os.remove(dst)
    os.rename(path, dst)
    print('RESULT rendered', dst)
