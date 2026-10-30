<!-- SPDX-License-Identifier: MIT -->
# GLCE Sample Robot v5 — Independent Base Motor Mount

## Design intent

v5 implements a separate **`base_motor_mount.stl`** for the connection between the base motor output shaft and the rotating support for the shoulder. This specifically addresses the v4 visual collision: the upright structural riser penetrated the shared base motor housing.

The motor remains a single **unchanged `motor.stl`** reused at four sites. Instead of moving the motor or changing the kinematic joint centers, v5 separates the *fixed base support* from the *rotating mount*.

```text
World / Fixed frame
  ├── base_support.stl       foundation below motor (static)
  └── motor.stl              base motor housing (static)
         +Z output shaft
          ↓
Base yaw [1] (rotating)
  └── base_motor_mount.stl  hub above motor + offset riser + shoulder shelf
       └── Shoulder motor mount [10] and Shoulder [2]
```

The `base_motor_mount` is a conceptual coupling bracket, not a production motor-mount specification. Its hub begins at **Z = +23.5 mm**, which is the top of the `motor.stl` output shaft; the motor housing is unchanged and the upright support is kept above it. `base_support.stl` now terminates at **Z = −15.5 mm**, touching the motor's bottom rear cap instead of intersecting its housing. All units are millimeters.

## What's changed from v4

| Property | v4 | v5 |
|---|---|---|
| Shared motor STL | 1 used 4 times | **Unchanged** |
| Fixed base geometry | `base_support` includes riser, intersects motor | `base_support` contains only footing |
| Rotating base structural support | part of base support | **new `base_motor_mount.stl`** |
| Joint IDs 1–8 / positions | existing | **unchanged** |
| Base motor frame [9] parent | 1 (yawing) | 0 (stationary) |
| Base support frame | 1 (yawing) | **new fixed frame [13]**, parent 0 |
| Base motor mount mesh frame | — | **[1]**, moves with yaw |
| Unique ASCII STLs | 9 | **10** |
| Mesh entries | 12 | **13** |

## Kinematic coordinate tree

```text
Root / World [0]
  ├── Fixed Foundation [13]       → base_support.stl
  ├── Fixed Base Motor [9]        → motor.stl (common geometry)
  └── Base Yaw [1]                → base_motor_mount.stl
       ├── Shoulder Motor [10]    → motor.stl (axis +Y)
       └── Shoulder Pitch [2]     → shoulder_link.stl
            ├── Elbow Motor [11]  → motor.stl (axis +Y)
            └── Elbow Pitch [3]   → elbow_link.stl
                 ├── Wrist Motor [12] → motor.stl (axis +X)
                 └── Wrist Roll [4]   → wrist_link.stl and tool_interface.stl
                      └── Tool Mount [8] → gripper_body.stl
                           ├── Left Finger [5] → left_finger.stl
                           ├── Right Finger [6] → right_finger.stl
                           └── TCP [7] → no mesh
```

The original joint positions and rotations remain: shoulder `(0, 0, 140)`, elbow `(150, 0, 0)`, wrist `(120, 0, 0)`; Tool Mount `[8]` remains `(26, 0, 0)` relative to Wrist `[4]`. All finger and TCP locations are retained. Four motor entries still request `geometry = motor` and `material = actuator_mat`, so the Geometry Registry and Material Registry can share their resources.

The mount, foundation, and arm structures are assigned `link_mat`; the tool interface and motor housings use `actuator_mat`; the gripper uses `end_tool_mat`.

## Files and integration

- `stl/` contains 10 ASCII STL files. The only regenerated geometry is `base_support.stl` and the new `base_motor_mount.stl`. The other 8 files (including `motor.stl`) are byte-for-byte identical to v4.
- `robot_sample_v5.glce_model` specifies the 13 Mesh entries and the fixed/rotating frames. It is an *example for adapting the C dummy loader*; GLCE does not currently load this file directly for the sample robot.
- `source/robot_v5.scad` contains parametric source and an assembly preview; `source/export.sh` regenerates the ASCII STLs using OpenSCAD.
- `validation_report.csv` records triangle topology checks.

### Verification scope

Each STL was checked to be ASCII STL, watertight, single-component, consistently wound, and of positive signed volume. This does **not** prove collision-free motion, shaft fit/tolerances, screw hole alignment, or real-world mechanical functionality. The base motor/structural part separation addresses the specific v4 penetration in the zero pose.

This package is visual/educational sample geometry, **not manufacturing CAD**.
