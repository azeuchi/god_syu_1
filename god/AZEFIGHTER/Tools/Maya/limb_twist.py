# Keep IK arms and legs from spinning around themselves.
#
# A pole locator placed in hips space on each key pose does not follow the limb's own axis. When a hand
# or foot swings (guard -> punch -> guard, crouch -> kick -> crouch) the elbow / knee direction around the
# root -> end line can turn by 50-100 degrees in a frame, or all the way round over a move, and the limb
# reads as twisting.
# A fist or a kick travels without the elbow / knee turning around the limb, so the direction of the
# first frame is carried along with the limb: every frame it is turned by the smallest rotation that takes
# last frame's limb axis to this frame's (in the chest frame for arms, the hips frame for legs). The fist /
# foot's roll around the forearm / shin is carried the same way, and a planted foot keeps its world
# orientation so it does not turn on the floor.
# Carried around a closed path a direction does not come back exactly where it started, so the difference
# to the last frame's pose is spread over the whole move and the clip ends exactly in the pose the next
# clip starts from.
# check_arm_roll.py / check_leg_roll.py measure the twist per frame, check_loop_end.py the end pose.
import math
import maya.cmds as cmds
import maya.api.OpenMaya as om

NS = 'mixamorig:'
CHEST = NS + 'Spine2'
HIPS = NS + 'Hips'
POLE_DISTANCE = 40.0


def _matrix(node):
    return om.MMatrix(cmds.xform(node, q=True, ws=True, matrix=True))


def _to_local(p, frame_inv):
    q = om.MPoint(p.x, p.y, p.z) * frame_inv
    return om.MVector(q.x, q.y, q.z)


def _ws(node):
    return om.MVector(*cmds.xform(node, q=True, ws=True, t=True))


def _perpendicular(v, axis):
    a = axis.normal()
    return (v - a * (v * a)).normal()


def _signed_angle(a, b, axis):
    """Angle turning a onto b around axis."""
    return math.atan2((a ^ b) * axis.normal(), a * b)


def _ease(t):
    return t * t * (3.0 - 2.0 * t)


def _carry(axes, start_dir):
    """Carry start_dir along a list of axes; returns the direction on every frame."""
    out = []
    prev_axis, d = None, start_dir
    for axis in axes:
        if prev_axis is not None:
            d = _perpendicular(d.rotateBy(om.MQuaternion(prev_axis, axis)), axis)
        out.append(d)
        prev_axis = axis
    return out


def _closed(dirs, axes, end_dir):
    """Spread the miss at the last frame over the move so the last direction is end_dir."""
    miss = _signed_angle(dirs[-1], end_dir, axes[-1])
    n = len(dirs) - 1
    return [d.rotateBy(om.MQuaternion(miss * _ease(i / float(n)), a)) for i, (d, a) in enumerate(zip(dirs, axes))]


def reference_frame(start_joint):
    """Arms are measured against the chest, legs against the hips."""
    return HIPS if 'UpLeg' in start_joint else CHEST


def hold_bend_direction(handle, pole, start_joint, mid_joint, end_frame):
    """Re-key pole on every frame so the elbow / knee keeps its direction around the limb.

    The directions of the first and last frames are the ones the keyed pole gives there
    (the guard, or the stance the next clip starts from); frames between only carry them along."""
    ref = reference_frame(start_joint)
    frames = range(0, int(end_frame) + 1)
    samples = []
    for f in frames:
        cmds.currentTime(f)
        m = _matrix(ref)
        m_inv = m.inverse()
        root = _to_local(_ws(start_joint), m_inv)
        end = _to_local(om.MVector(*cmds.getAttr(handle + '.translate', time=f)[0]), m_inv)
        samples.append((m, root, end - root))
        if f == 0:
            first_mid = _to_local(_ws(mid_joint), m_inv) - root
        if f == frames[-1]:
            last_mid = _to_local(_ws(mid_joint), m_inv) - root
    axes = [s[2] for s in samples]
    dirs = _closed(_carry(axes, _perpendicular(first_mid, axes[0])), axes, _perpendicular(last_mid, axes[-1]))

    cmds.cutKey(pole, attribute=['translateX', 'translateY', 'translateZ'], clear=True)
    for f, (m, root, axis), d in zip(frames, samples, dirs):
        local = root + axis * 0.5 + d * POLE_DISTANCE
        p = om.MPoint(local.x, local.y, local.z) * m
        cmds.setKeyframe(pole, attribute='translateX', time=f, value=p.x)
        cmds.setKeyframe(pole, attribute='translateY', time=f, value=p.y)
        cmds.setKeyframe(pole, attribute='translateZ', time=f, value=p.z)


def hold_end_roll(start_joint, mid_joint, end_joint, end_frame):
    """After baking: turn the fist / foot around the forearm / shin so it does not roll.

    The solver bends the elbow / knee on more than one axis, so the lower limb can roll a lot while it
    straightens even when the bend direction is held. Turning around the lower limb's own line does not
    move the wrist / ankle, only how the fist / foot faces. The fist is turned with the forearm (the
    forearm really twists), the foot on its own ankle (the shin does not)."""
    ref = reference_frame(start_joint)
    turn_joint = end_joint if 'UpLeg' in start_joint else mid_joint
    frames = range(0, int(end_frame) + 1)

    # the end joint's local axis that lies most across the lower limb at frame 0 shows its roll
    cmds.currentTime(0)
    m0 = _matrix(end_joint)
    along = (_ws(end_joint) - _ws(mid_joint)).normal()
    row = min(range(3), key=lambda r: abs(om.MVector(m0[r * 4], m0[r * 4 + 1], m0[r * 4 + 2]).normal() * along))

    def sample(f):
        cmds.currentTime(f)
        m_inv = _matrix(ref).inverse()
        m = _matrix(end_joint)
        axis = (_ws(end_joint) - _ws(mid_joint)) * m_inv
        roll = _perpendicular(om.MVector(m[row * 4], m[row * 4 + 1], m[row * 4 + 2]) * m_inv, axis)
        return axis, roll

    axes, rolls = zip(*[sample(f) for f in frames])
    wanted = _closed(_carry(axes, rolls[0]), axes, rolls[-1])
    for f, axis, want in zip(frames, axes, wanted):
        _, roll = sample(f)
        delta = _signed_angle(roll, want, axis)
        if abs(delta) < 1e-4:
            continue
        world_axis = axis * _matrix(ref)
        pivot = _ws(turn_joint)
        wm = om.MTransformationMatrix(_matrix(turn_joint))
        wm.rotateBy(om.MQuaternion(delta, world_axis), om.MSpace.kWorld)
        wm.setTranslation(pivot, om.MSpace.kWorld)
        cmds.xform(turn_joint, ws=True, matrix=list(wm.asMatrix()))
        cmds.setKeyframe(turn_joint, attribute=['rotateX', 'rotateY', 'rotateZ'], time=f)


def hold_foot(foot_joint, end_frame, planted):
    """After baking: the foot keeps its frame-0 orientation.

    A planted foot keeps it in the world so it does not turn on the floor. A kicking foot keeps it
    relative to the hips: the ankle stretches as the shin swings forward and the toes stay pointed
    along the kick, instead of the foot swinging and rolling with the shin."""
    def frame_matrix():
        return om.MMatrix() if planted else _matrix(HIPS)

    cmds.currentTime(0)
    base = _matrix(foot_joint) * frame_matrix().inverse()
    for f in range(1, int(end_frame) + 1):
        cmds.currentTime(f)
        wm = om.MTransformationMatrix(base * frame_matrix())
        pivot = _ws(foot_joint)
        wm.setTranslation(pivot, om.MSpace.kWorld)
        cmds.xform(foot_joint, ws=True, matrix=list(wm.asMatrix()))
        cmds.setKeyframe(foot_joint, attribute=['rotateX', 'rotateY', 'rotateZ'], time=f)


def smooth_foot(foot_joint, end_frame, passes=4):
    """After baking: spread sudden foot turns over the neighbouring frames (first and last frame kept).

    At a takeoff the shin swings in a frame or two and a foot fixed to it snaps from flat to pointed.
    Smoothing its orientation (relative to the hips) turns that snap into a toe-off over a few frames."""
    frames = range(0, int(end_frame) + 1)
    rel = []
    for f in frames:
        cmds.currentTime(f)
        rel.append(om.MTransformationMatrix(_matrix(foot_joint) * _matrix(HIPS).inverse()).rotation(asQuaternion=True))
    for _ in range(passes):
        rel = [rel[0]] + [om.MQuaternion.slerp(om.MQuaternion.slerp(rel[i - 1], rel[i + 1], 0.5), rel[i], 0.5)
                          for i in range(1, len(rel) - 1)] + [rel[-1]]
    for f, q in zip(frames, rel):
        cmds.currentTime(f)
        wm = om.MTransformationMatrix(q.asMatrix() * _matrix(HIPS))
        wm.setTranslation(_ws(foot_joint), om.MSpace.kWorld)
        cmds.xform(foot_joint, ws=True, matrix=list(wm.asMatrix()))
        cmds.setKeyframe(foot_joint, attribute=['rotateX', 'rotateY', 'rotateZ'], time=f)
