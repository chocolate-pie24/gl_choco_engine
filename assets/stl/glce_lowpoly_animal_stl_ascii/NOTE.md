# GLCE Low-Poly Animal STL Test Assets

These ASCII STL files were generated with assistance from ChatGPT as original procedural/geometric test meshes for GLCE.

Purpose:
- STL loader testing
- ASCII STL parser testing
- basic renderer verification
- facet-normal verification
- low-poly lighting checks
- normal quantization comparison tests

Coordinate convention:
- Z-up
- X axis is the main front/back animal direction
- Units are arbitrary GLCE test units

Important note:
- These are renderer/test assets, not production CAD assets.
- Each model is built from multiple closed low-poly primitive components.
- Some components intentionally overlap instead of being boolean-unioned into a single manifold solid.
- This is suitable for STL loading and rendering tests.
- Do not treat these as guaranteed 3D-print-ready watertight single-shell models.

Origin:
- Generated with assistance from ChatGPT.
- Designed as original GLCE test meshes.
- Not derived from external 3D model files, scanned objects, third-party sample meshes, or copyrighted characters.

License:
- Unless otherwise specified by the GLCE project, these generated test assets may be distributed under the same license as GLCE.
