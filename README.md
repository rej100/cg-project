# 3DCGA Project

## How to run

### Requirements

- A C++20 compiler, such as GCC.
- CMake 3.14 or newer.
- Ninja for the terminal workflow below.
- OpenGL 4.1 support, the platform's OpenGL/window-system development libraries,
  and a graphical desktop session.
- For the VS Code workflow: the CMake Tools extension.

### From the terminal (Linux)

Run these commands from the repository root, which contains `CMakeLists.txt`:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target Master_TechDemo
./build/Master_TechDemo
```

The first command configures the project by generating build files in `build/`.
The second compiles the executable, and the third launches it.

After ordinary C++ changes, rebuild and run:

```bash
cmake --build build --target Master_TechDemo
./build/Master_TechDemo
```

Run the configuration command again after changing `CMakeLists.txt` or adding
shader files. GLSL shaders compile when the application starts; check the
terminal for shader-loading or compilation errors.

### From VS Code

Open the repository root in VS Code. In the Command Palette (`Ctrl+Shift+P`),
use the following tested sequence:

1. **CMake: Select a Kit** — select the GCC kit.
2. **CMake: Select Variant** — select **Debug**.
3. **CMake: Set Build Target** — select **Master_TechDemo**.
4. **CMake: Run Without Debugging** — build and launch the application.

CMake Tools manages configuration as needed during this workflow. Configuring
means generating the build files from `CMakeLists.txt`; building then compiles
the program using those files. For subsequent runs, use **CMake: Run Without
Debugging** again.

### Entry point and current behavior

The executable target is `Master_TechDemo`, defined in the root `CMakeLists.txt`.
Its C++ entry point is `main()` in `src/application.cpp`, which constructs an
`Application` and calls `update()` to run the input/rendering loop.

The application opens **Reactor Inspection** and loads these model assets:

| Model | Geometry | Placement |
| --- | --- | --- |
| `assets/models/platform.obj` | 12 m diameter, 0.38 m tall; 15,352 triangles in three material groups. | Identity transform, with its bottom at ground level. |
| `assets/models/reactor.obj` | 3.5 m tall; 61,820 triangles in six material groups. | Translated by `(0, 0.38, 0)` to sit on the platform. |
| `assets/models/drone_body.obj` | Textured drone body. | Root translation `(0, 4.38, 4)` and uniform scale 4. |
| `assets/models/L_F_propeller.obj`, `R_F_propeller.obj`, `L_B_propeller.obj`, `R_B_propeller.obj` | Four separately loaded propeller models. | Drone root transform composed with each propeller's local offset. |
| `assets/models/placeholder.obj` | Orange box/arrow spotlight marker; 30 triangles in one material group. | Its box center follows the spotlight position, with its arrow aligned to the light's aim. |

The drone is a static assembly contributed through the partner's drone branch.
Its root transform and propeller offsets are retained from that contribution;
all seven exhibit objects are created explicitly and added to the scene vector.
The [asset inventory](assets/README.md#drone) lists its files, textures, and offsets.

The OBJ/MTL files supply material colors and base-color textures, including the
marble, gold-colored trim, ceramic housing, and TU Delft logos. Metallic (`Pm`)
and roughness (`Pr`) constants are copied into each GPU material, and roughness
images (`map_Pr`) are loaded and uploaded as linear/non-color textures for
PBR shading. Missing `Pm` and `Pr` use tinyobjloader's existing
default of 0. Materials that reference a base-color texture without an explicit
`Kd` (the marbles and reactor panel; Blender omits constants for connected
texture inputs) import tinyobjloader's documented gray fallback (0.6, 0.6,
0.6); we fixed a vendored-copy bug where `has_kd` leaked between materials and
skipped that fallback. The cyan core also supplies linear emission through its
MTL `Ke` value. Startup output lists each model and material, triangle counts,
base-color and roughness texture availability, metallic and roughness
constants, and any nonzero emission values.

### Scene controls

| Control | Action |
| --- | --- |
| Left mouse drag over the scene | Orbit around the camera's target. |
| Right mouse drag over the scene | Pan the camera and its target. |
| Mouse wheel over the scene | Zoom in or out. |
| **Scene overview** | Frame the platform and reactor from above their front side (+Z). |
| **Reactor** | Frame the reactor more closely. |
| **Reactor panel** | Inspect the front logo and `REACTOR 1337 / GROUP 4` caption. |
| **Platform logo** | Frame the logo near the platform's front rim. |
| **Shading** | Select **Lit** (diffuse reference), **PBR**, **Base color**, or **Normals**. |
| **Material textures** | Sample each material's base-color/roughness images; when off, use the material's loaded constant values. |
| **Normal mapping** | Shade with the tangent-space normal maps' relief (platform logo, reactor panel); when off, use the original mesh normals. |
| **Environment reflections (PBR)** | Toggle the environment's specular reflection in **PBR** mode; the sky background stays visible. |
| **Emission** | Toggle the core's self-luminous contribution in **Lit** and **PBR** modes. |
| **Lighting → Sun / Intensity** | Toggle the directional sunlight and adjust its intensity on one row. |
| **Lighting → Spotlight / Intensity** | Toggle the showcase spotlight and adjust its intensity on one row. |
| **Lighting → Ambient strength** | Adjust the single constant ambient-fill contribution; default 0.25. |
| **Lighting → Show light marker** | Show the orange box/arrow at the spotlight's position, pointing toward its target. |

**Lit** adds directional sunlight, spotlight illumination, and one adjustable
ambient-fill term, then adds material emission when enabled. All contributions
are accumulated in linear RGB. Emission is independent of the surface's
angle to the light. The core's `Ke (0, 1, 1.8)` already includes its authored
strength and is used directly. The current SDR output clips channels above 1;
HDR rendering and bloom are planned for preserving excess brightness and adding
a halo.
**Base color** displays the material color or texture without lighting.
**Normals** displays the surface normals actually used for shading, mapping
XYZ components from [-1, 1] into RGB [0, 1]. The emission toggle affects only
**Lit** and **PBR** modes.

### PBR shading and the material-textures toggle

**PBR** replaces the simple diffuse response with a metallic/roughness
Cook-Torrance BRDF: GGX specular highlights, Smith visibility, Schlick fresnel,
and Lambert diffuse scaled down by metallic. Metals use their base color as the
specular F0, while nonmetals reflect about 4%. Roughness is clamped to a small
minimum (0.045) so zero-roughness materials stay numerically well-defined. The
same function evaluates the sun and the spotlight; ambient fill and emission
accumulate as in **Lit** mode. Environment reflections add a view-dependent
specular contribution when enabled.

The **Material textures** checkbox chooses the source of the shading inputs
independently of the shading formula. When on, each material samples its
available base-color and roughness images; when off, it uses the constants
loaded from the MTL: gold keeps its authored gold color, metallic 1, and
roughness 0.21, while textured marble falls back to its imported gray base
color (0.6, 0.6, 0.6) and roughness 0. Sampled roughness only affects **PBR**;
the diffuse **Lit** reference has no roughness-dependent response.

### Normal mapping

The normal map replaces the interpolated mesh normal with a more detailed
surface direction sampled from an image, so the baked relief responds to
lighting while the mesh silhouette stays unchanged. The mesh's tangent basis is
generated once at load time from the triangle positions and UVs: one
`tangentAndBitangentSign` record per vertex (unit tangent direction plus a +1/−1
bitangent sign, uploaded as vertex attribute 3). For each fragment, the shader
re-orthogonalizes the interpolated tangent against the interpolated mesh
normal, reconstructs the bitangent, and builds a temporary tangent-to-world
basis that converts the sampled direction into world space:

```glsl
mappedNormal = texture(normalMap, uv).rgb * 2.0 - 1.0; // [0,1] -> [-1,1]
mappedNormal.xy *= normalMapStrength;                  // authored -bm strength
shadingNormal = normalize(tangentToWorld * mappedNormal);
```

The resolved normal feeds the **Lit**, **PBR**, and **Normals** paths. The
platform logo uses strength 1.25 and the reactor panel 1.5, exported as
`map_Bump -bm ...` and loaded as linear/non-color data. Materials without a
normal map, and fragments whose UVs carry no usable tangent, fall back to the
mesh normal. The **Normal mapping** checkbox is independent of **Material
textures**: artwork visibility and relief shading can be toggled separately,
and the **Normals** diagnostic displays whichever normal is in use.

### Environment mapping

The application loads Poly Haven's **Qwantani Noon (Pure Sky)**, a 2048 × 1024
CC0 HDR panorama. Its float RGB values represent the distant surroundings in
every direction. At startup, `Environment` renders the panorama into six
1024 × 1024 cubemap faces. Longitude/latitude lookups are only needed during
this conversion; the sky and reflections then sample the cubemap by a
world-space direction. The cubemap uses `GL_RGB32F` to preserve the source's
bright sun, whose values exceed the half-float range. The original download,
authors, and license are recorded in the [asset inventory](assets/README.md#environment).

Application setup calls `Environment::initialize()` once, after creating the
OpenGL context. Initialization explicitly reads and restores the OpenGL settings
used during conversion. The cubemap and cube geometry remain allocated until
context destruction at shutdown; initialization errors reach `main()`, which
prints the error and exits.

For the background, a small code-generated cube follows the camera's position:
its transform uses view rotation but omits translation. It is drawn first with
depth writes disabled, so scene objects can cover it at any distance. The sky
therefore turns with the view but has no positional parallax, representing an
infinitely distant environment.

In **PBR**, the same environment supplies reflections:

```glsl
vec3 reflectedDirection = reflect(-viewDirection, shadingNormal);
vec3 reflectedColor = textureLod(environmentMap, reflectedDirection,
    clamp(roughness, 0.0, 1.0) * environmentMaxLOD).rgb;
```

Schlick Fresnel weights the result, with base-color F0 for metals and 0.04 for
nonmetals. Normal-mapped details affect the reflected direction too. Ordinary
cubemap mipmaps provide an inexpensive roughness-dependent blur approximation;
they are not a GGX-prefiltered specular IBL solution. The constant ambient fill
continues to supply diffuse fill. Reflections contain this distant panorama,
not the reactor or drone; the cubemap is bound once per frame on texture unit 3
(base color, roughness, and normal maps occupy units 0, 1, and 2).

To inspect the effect, choose **PBR**, use the **Reactor** camera, orbit around
the curved gold rings/collar, and toggle **Environment reflections (PBR)**.
The reflected sky shifts across the gold while the background and direct
lighting stay visible. **Lit**, **Base color**, and **Normals** do not add
environment reflections. Both the sky and scene shaders convert linear RGB to
sRGB for the current SDR output, so values above 1 still clip on display; the
floating-point environment data is retained for the planned HDR/bloom pipeline.

### Lighting and its marker

The sun supplies white directional light with direction-to-light
`(-0.4, 1, 0.6)` and default intensity 0.75. The spotlight starts at
`(5, 6, 4.5)`, aimed at `(0, 1.8, 0)`, with white light, intensity 0.3,
and a single cone half-angle of 25° (50° full opening). Its direction is derived
from `target - position`. Position, target, light colors, and cone angle are
configured in `src/light.h`; settings are uploaded as ordinary uniforms.

Spotlight illumination uses the world-space fragment-to-light direction and
a simple cone test: fragments outside the cone receive zero spotlight light.
Inside the cone, incoming light is `color * intensity`, with a hard boundary
and constant strength with distance. Surface shading still depends on the
normal's angle to the light. Coincident position/target values use a downward
aim, and a fragment at the light position receives zero spotlight contribution.

The Lighting section has four compact rows: sun enable/intensity, spotlight
enable/intensity, ambient strength, and marker visibility. Both intensity
sliders use the range 0–2. Widget widths and spacing follow the UI font size.

The ambient strength starts at 0.25 and is added once, independently of the
two direct lights. Future environment-based diffuse lighting will replace this
constant fill. Shadow mapping is a later stage, using an orthographic map for
the sun and a perspective map for the spotlight.

`assets/models/placeholder.obj` is a Blender-authored, 30-triangle box/arrow
marker, loaded through the same OBJ/MTL pipeline as the exhibit models. Its
editable source is `assets/source/placeholder.blend`. The marker's `Model` owns
the GPU mesh, and an explicit `SceneObject` gets its transform from the
spotlight's position and aim. The marker points along local −Z;
the inverse light-view matrix places it in world space, with an alternate up
axis for vertical aiming. It is drawn after the exhibit objects using the
existing **Base color** path, then the selected shader mode is restored.
The marker stays colored in every shading mode and with the lights disabled,
while normal depth testing lets scene geometry obscure it.

To compare the lights, use **Lit**, turn **Emission** off, and set
**Ambient strength** to zero. Inspect the sun alone, the spotlight alone,
and both together. With both lights off, the exhibit surfaces are black;
the background and enabled debug marker remain visible. Changing the configured
spotlight position or target updates both the illumination and marker placement.
**PBR** uses the metallic/roughness data and roughness textures loaded from the
MTL files, while **Lit** provides the diffuse reference. Normal mapping applies
the baked logo/panel relief, and **PBR** also samples the environment for reflections.
Base-color textures are uploaded as sRGB and decoded by OpenGL; lighting uses
linear colors, and the scene shader converts its final output to sRGB. Image
rows are flipped during GPU upload to match the OBJ UV convention.

The main rendering responsibilities are:

| Code | Responsibility |
| --- | --- |
| `framework/src/mesh.cpp` | Read OBJ geometry, material names/properties, and referenced images; generate per-vertex tangents. |
| `src/model.h` | Define `Model`, which owns GPU meshes, and `SceneObject`, which references a model and supplies its transform. |
| `src/light.h` | Define directional/spotlight settings and derive the spotlight's aim direction. |
| `src/texture.cpp` | Upload images with explicit color space and the correct row orientation. |
| `framework/src/image.cpp` | Load 8-bit material images and linear float HDR panoramas. |
| `src/environment.cpp` | Convert the panorama into a cubemap, generate reflection mipmaps, own the cube resources, and draw the sky background. |
| `src/mesh.cpp` | Own GPU geometry/material resources and bind each mesh's base-color, roughness, and normal textures when drawing. |
| `src/application.cpp` | Own models, instances, lights, and environment; coordinate camera/UI, draw the sky first, upload shared settings and per-object transforms, and draw the light marker. |
| `shaders/shader_{vert,frag}.glsl` | Transform vertices and tangents, choose material inputs, resolve the shading normal, and apply ambient fill, emission, direct lights, and PBR environment reflections. |
| `shaders/skybox_vert.glsl`, `environment_convert_frag.glsl`, `skybox_frag.glsl` | Share the direction-supplying cube vertex shader between panorama conversion and sky rendering. |

The loaded models are stable `Application` members. Scene objects
reference those models, allowing one loaded model to be drawn at several
placements. `drawObject()` uploads each instance's model matrix and its
inverse-transpose normal matrix before drawing the model's material groups.

Emission follows `MTL Ke` → CPU `Material::emission` → `GPUMaterial` uniform
buffer → the fragment shader's `Material` block. It is added in linear RGB
before the final sRGB conversion.

### Window size and UI scaling

- The application starts **maximized**. Restore it using the window decorations
  to use its initial **1600 × 900** window size, then resize it normally.
- The rendering viewport and camera aspect ratio follow the actual framebuffer
  dimensions, preserving the scene's proportions as the window changes size.
  Rendering pauses while minimized and resumes when the window is restored.
- ImGui uses the operating system's DPI/content scale, with an **18-pixel base
  font at 100% scale**. For example, a detected display scale of 2× produces a
  36-pixel font with the default UI setting. The startup console reports both
  the detected scale and initial font size.
- The **Scene controls** panel shows **Display scale** and provides a **UI scale** slider
  from **0.75× to 2.0×** for personal adjustment. This multiplier starts at 1.0×
  each launch and applies on top of the detected display scale:

  ```text
  Font size = 18 px × display scale × UI scale
  ```

- Font glyphs are rasterized at the requested size, and widget spacing scales
  with them. When the slider changes or GLFW reports a new display scale, the
  font texture and layout are rebuilt between frames.

The root `build/` directory and generated `imgui.ini` window-layout file are
ignored by Git.

## Scene plan

### Concept: Reactor Inspection

A bright, clean energy exhibit in an open-air pavilion. A white-and-gold reactor
stands at the center of a white marble circular platform, while a small
inspection drone flies around it. The visual direction is a premium technology
showroom: simple shapes, polished marble with subtle grey veins, satin ceramic,
gold trim, a bright sky, and restrained cyan accents. Manufactured panels,
grooves, and markings add detail to the reactor and drone.

The scene runs automatically in one application. Camera presets and independent
UI controls let us inspect each graphics technique, pause the scene, and capture
matching before/after screenshots. The following is the planned scope, rather
than a list of features already implemented.

### Layout and animation

- **Exhibit platform:** approximately 12 m across, raised slightly above a broad,
  pale ground plane. Its polished white marble top has restrained grey veining
  and a narrow gold outer edge. An original-color TU Delft logo near the front
  rim marks its orientation and has deliberately pronounced normal-mapped bevels.
  It receives shadows from the reactor and drone.
- **Reactor:** approximately 3.5 m tall, with a white marble cylindrical pedestal
  and gold collar, a luminous cyan core, a dark inner recess, white ceramic
  housing panels, three gold rings, and three structural supports. One
  outward-facing panel prominently displays a blue TU Delft logo with a shallow,
  beveled, embossed appearance produced by its normal map, alongside grooves and
  screw heads. Keep the area around the logo uncluttered and frame it clearly in
  the showcase and material close-up views. The pedestal provides a close-up of
  marble beside gold.
- **Inspection drone:** approximately 1.2 m wide, with a white body, gold trim,
  four rotor assemblies, and a dark articulated sensor. It follows a closed loop
  around the reactor, approximately 3–4.5 m from its center, with gentle changes
  in height. Its blades spin, its sensor mount turns, and its sensor head tilts.
- **Showcase lamp:** a fixed, approximately 6 m tall mast beside the platform,
  with an angled overhead arm and a luminous lamp face. Its spotlight points
  across the reactor and intersects part of the drone route.
- **Environment:** a bright daylight sky with distant features that give metal
  reflections recognizable structure. The same environment supplies the visible
  background and environment reflections.
- **Lighting:** a stationary directional sun and a stationary showcase spotlight
  illuminate the scene from different angles. Both cast shadows. Balance them
  so the spotlight makes a visible contribution while the overall scene stays
  bright.
- **Energy effects:** a small stream of cyan particles rises through and just
  above the reactor core. The dark inner recess provides contrast. Bloom gives
  the core, lamp face, and selected particles a restrained halo.

The drone route has four cubic Bézier segments with smooth joins, including the
closing join. Its heading follows the route. A chosen pause position places the
drone inside the spotlight beam with both light-source shadows visible on the
platform.

### Scene inventory

Current asset files, previews, and export settings are documented in the
[asset inventory](assets/README.md).

#### Models and geometry

Create the scene models in Blender or obtain suitable models online, then use
Blender to prepare their geometry, materials, scale, and UVs. UVs describe how 2D
texture images map onto each model's surface. Export meshes as OBJ files and
textures as separate PNG images for the application to load.

The drone's individual parts are also authored assets. Its parent–child hierarchy,
local transforms, and animation are constructed in application code to satisfy
the hierarchical-transformation requirement. Export its moving parts separately
with appropriate rotation pivots.

| Object | Required parts | Construction / animation |
| --- | --- | --- |
| Ground | One broad ground plane | Static plane with a uniform pale material. |
| Exhibit platform | Circular base, marble top surface with TU Delft logo near the front rim, narrow gold edge | Authored static mesh with small edge bevels; the marble top has platform-specific color and normal maps, with separate materials for the gold edge and white base. |
| Reactor pedestal | Marble base cylinder and gold collar | Part of the authored reactor asset; shares the platform's marble texture set at a consistent world-space scale. |
| Reactor core | Cyan inner cylinder and dark backing/recess | Part of the authored reactor asset; core has an emissive material. |
| Reactor housing | Three white outer panels and one visible detail panel bearing the TU Delft logo | Authored panels with readable UVs for the colored logo, its normal-mapped bevels, grooves, and markings. |
| Reactor structure | Three gold rings and three supports | Static parts included in the reactor asset, with beveled exposed edges. |
| Drone body | Main shell, trim, four rotor mounts, and fixed hubs | Authored body mesh; its root transform follows the path and is inherited by the other parts through a hierarchy built in code. |
| Drone rotors | Four two-blade assemblies | One authored blade-assembly mesh reused four times; code spins each instance around its mount. Blade shape makes rotation visible. |
| Drone sensor | Turning mount, tilting head, and dark lens | Separate mount and head meshes; code constructs body → turning mount → tilting head, with the lens included in the head mesh. |
| Showcase lamp | Mast, overhead arm, lamp housing, luminous face | Authored static mesh; the spotlight created in code matches the fixture's position and direction. |
| Sky background | Camera-centered cube | Small code-generated rendering helper that samples the environment cubemap. |
| Energy particles | Reusable camera-facing quad | Code-generated rendering helper; particle instances have position, velocity, age, size, and opacity. |
| Path visualization | Sampled line along the four Bézier segments | Generated from the actual flight path; UI-controlled visibility. |

The ground plane, skybox cube, particle quads, and path lines are simple rendering
helpers generated in code. The exhibit and drone geometry come from asset files.

#### Model exports and editable source files

Keep editable originals in `assets/source/`, exported meshes in `assets/models/`,
and exported textures in `assets/textures/`. Use one Blender file per major asset
so the two group members can work on different assets independently. Keep source
files and exports in version control; the application loads the exported meshes
and images without requiring Blender at runtime.

| Planned file | Contents / use |
| --- | --- |
| `assets/source/platform.blend` | Editable platform geometry, UVs, marble/gold material setup, and texture-baking setup. |
| `assets/source/reactor.blend` | Editable reactor, pedestal, and detail panel, including the detailed logo geometry used for baking. |
| `assets/source/drone.blend` | Editable body, rotor, sensor mount, and sensor head meshes, with reference positions and pivots for assembly. |
| `assets/source/showcase_lamp.blend` | Editable lamp geometry and material assignments. |
| `assets/source/TU_Delft_Logo.svg` | User-provided vector source for the original-color logo artwork and beveled relief; source and authorship recorded in `assets/README.md`. |
| `assets/models/platform.obj` | Static platform, with material groups for its marble top, base, and gold edge. |
| `assets/models/reactor.obj` | Static reactor assembly: pedestal, collar, core, recess, panels, rings, and supports, with named material groups. |
| `assets/models/showcase_lamp.obj` | Static fixture with separate housing and emissive-face material groups. |
| `assets/models/drone_body.obj` | Shell, trim, fixed rotor mounts, and hubs. |
| `assets/models/drone_rotor.obj` | One two-blade assembly centered on its rotation pivot, instantiated four times. |
| `assets/models/drone_sensor_mount.obj` | Turning sensor mount centered on its yaw pivot. |
| `assets/models/drone_sensor_head.obj` | Tilting sensor head and lens, centered on its tilt pivot. |

OBJ files contain mesh coordinates rather than Blender's object origins or
parenting. Export each moving drone part with its pivot at `(0, 0, 0)` in its mesh
coordinates, and define its assembly offset relative to its parent in code.

Retain matching `.mtl` companions produced by OBJ export: `platform.mtl`,
`reactor.mtl`, `showcase_lamp.mtl`, `drone_body.mtl`, `drone_rotor.mtl`,
`drone_sensor_mount.mtl`, and `drone_sensor_head.mtl`, alongside their OBJ files.
Use material names to assign our PBR parameters and texture sets in the
application; Blender shader-node graphs are baked into images rather than
reconstructed from the MTL files. Record online asset sources, authors, licenses,
and any modifications in `assets/README.md` for attribution in the report.

#### Texture and environment files

These are planned paths for assets to create or obtain. Each texture set is
shared by the relevant object's parts; gold, aluminium, the sensor lens, and
emissive surfaces can use constant material parameters instead of extra images.

| Planned file | Purpose |
| --- | --- |
| `assets/textures/marble_basecolor.png` | Reusable polished white marble with subtle grey veins, used for the reactor pedestal and as the source for the platform's marble composition. |
| `assets/textures/marble_roughness.png` | Low roughness with subtle natural variation across the marble. |
| `assets/textures/marble_normal.png` (optional) | Very fine surface detail; veins remain color patterns rather than deep grooves. A flat normal is sufficient initially. |
| `assets/textures/platform_basecolor.png` | Marble with the original-color TU Delft logo positioned near the platform's front edge. |
| `assets/textures/platform_normal.png` | Deliberately pronounced beveled logo relief, with flat normals elsewhere; tangent-space +X/+Y/+Z data. |
| `assets/textures/reactor_basecolor.png` | White housing panels, the blue TU Delft logo, and diagnostic labels. |
| `assets/textures/reactor_roughness.png` | Satin housing surfaces and smoother selected details. |
| `assets/textures/reactor_normal.png` | Shallow beveled relief for the TU Delft logo, recessed grooves, and screw heads; especially clear on the detail panel. |
| `assets/textures/drone_basecolor.png` | White shell, colored identification markings, and small labels. |
| `assets/textures/drone_roughness.png` | Surface-finish variation across the shell and trim regions. |
| `assets/textures/drone_normal.png` | Panel seams and small manufactured details. |
| `assets/textures/particle_soft.png` | Soft particle sprite with an alpha falloff; tinted cyan during rendering. |
| `assets/environment/qwantani_noon_puresky_2k.hdr` | Poly Haven daylight panorama (CC0), converted to a cubemap for the background and reflections. |
| `assets/README.md` | Asset inventory, authorship, source links, licenses, and modifications, including our own generated assets. |

Base-color textures use sRGB color interpretation; roughness and normal maps are
linear data. Normal maps use a consistent tangent-space convention. When material
textures are disabled, use the constants supplied by the MTL loader, including
its defaults for missing properties; missing metallic and roughness are both 0.
Marble is a nonmetallic, opaque PBR material; its polished finish
contrasts with the satin ceramic housing. Gold trim uses separate geometry and a
metallic material, keeping the marble texture set simple.

For the TU Delft logo, use the vector outline to create shallow raised geometry
with beveled edges in Blender. Bake the detailed panel's tangent-space normals
onto the simple, UV-mapped runtime panel and combine its other details into the
same reactor normal map. Align the blue artwork in the base-color texture with
that relief. The runtime logo's apparent depth comes from normal mapping on the
simple panel, leaving its silhouette unchanged. With normal mapping disabled,
the blue logo remains visible as a flat printed design.

#### Asset preparation and first milestone

1. **Validate one asset end to end first:** use the platform's marble, gold trim,
   and normal-mapped TU Delft logo to test the Blender-to-application workflow.
   Its source, mesh export, textures, and comparison previews are available in
   the asset inventory; validate them in the renderer before expanding the scene.
2. **Prepare UVs and materials:** unwrap the model so the color, roughness, and
   normal textures align. Keep the logo readable and marble vein scale consistent
   between surfaces. Author materials or adapt suitable online texture sets.
3. **Bake textures:** export base color without lighting or shadows, roughness as
   linear data, and normal maps in tangent space. Bake procedural Blender
   materials into images using the planned texture paths.
4. **Export meshes:** use meters and a consistent Y-up axis convention, apply
   object rotation/scale, and center moving-part mesh coordinates on their pivots.
   Export triangulated geometry with normals, UVs, and named material groups.
   Bake normal maps against the same triangulated mesh and shading normals that
   will be exported.
5. **Verify in the application:** load the OBJ and texture files, generate the
   tangent basis from the mesh normals/UVs using conventions matching the bake,
   and check scale, orientation, material assignments, pivots, and normal-map
   direction. Confirm the logo's normal-map toggle works and that the marble and
   gold read as distinct materials.
6. **Build out the inventory:** use the verified conventions for the full reactor,
   platform, lamp, and separate drone parts. Keep editable Blender sources and
   exports synchronized as assets change.

#### Other required scene data and rendering resources

- **Asset loading and scene data:** load the model and texture files, then define
  object placement, material assignments, light settings, four Bézier segments,
  and camera presets in code. Construct the drone's parent–child relationships,
  pivot offsets, and component animation in code from its separate mesh assets.
- **Camera presets:** whole-scene showcase, overhead route view, drone-follow
  view, reactor material close-up, and drone mechanism close-up.
- **Material rendering:** PBR shading, an independently selectable simple
  reference shader mode, material-texture sampling, tangent-space normal mapping,
  and environment reflection sampling.
- **Environment preprocessing:** runtime conversion of the HDR panorama to a
  cubemap and roughness filtering for reflections.
- **Shadow rendering:** one depth map per light, with an orthographic projection
  for the sun and a perspective projection for the spotlight. Update both while
  the drone moves; share the depth-rendering code between the lights.
- **Effect rendering:** particle billboards, a path-line renderer, and an HDR
  scene target followed by bright-region extraction, blur, and bloom composition.
- **Runtime-generated data:** rendering-helper meshes, tangent data for imported
  meshes, path samples, the cumulative distance lookup table for constant-speed
  motion, particles, environment maps, shadow maps, and post-processing buffers.
  These are generated resources rather than additional asset files to download or
  commit.

### Planned mandatory features

| Feature | Where it appears | How we show it / UI controls |
| --- | --- | --- |
| Multiple viewpoints | The platform, flight route, drone, and reactor details. | Switch between showcase, overhead, follow, and close-up camera presets while the same scene runs. |
| Advanced shading: PBR | Polished marble beside gold trim, white ceramic, gold reactor rings, aluminium supports, and the dark sensor. Curved surfaces expose highlight shape and material differences. | Compare PBR against the simple reference shading mode at a fixed camera, light setup, and exposure. Include the marble pedestal and gold collar in a material close-up. |
| Material textures | Marble veining on the platform and pedestal, reactor labels, drone markings, and spatial roughness variation. | Toggle material textures against constant material values. Marble veins and printed markings disappear, and roughness becomes uniform per material. Keep normal mapping separately controllable. |
| Normal mapping | Pronounced beveled TU Delft logo on the platform, embossed logo and grooves/screw heads on the reactor panel, and drone panel detail. | Use the platform-logo or reactor close-up and toggle normal mapping while keeping material textures enabled. The logo changes from apparently raised and beveled to flat colored artwork; grooves and screw heads flatten too, while the mesh silhouette stays unchanged. |
| Environment mapping | Gold reactor rings and metallic drone trim reflect the sky and distant environment. | Move the camera around the curved metal and toggle its environment-reflection contribution. Keep the background and direct lighting visible. |
| Smooth paths using at least three cubic Bézier curves | The drone flies a closed route comprising four smoothly joined cubic segments. | Toggle path movement and the rendered curve independently. The overhead view exposes the route; the showcase view exposes height changes. |
| Hierarchical transformations | Code-assembled drone body → rotor mounts → spinning blades, and body → turning sensor mount → tilting sensor head, using separately loaded mesh assets. | Pause route movement while component animation continues, then toggle component animation separately. Use the drone close-up to show the nested motion. |

### Planned extra features

| Feature | Where it appears | How we show it / UI controls |
| --- | --- | --- |
| Constant-speed Bézier movement | The drone maintains approximately constant world-space speed across the complete route using a sampled cumulative-distance lookup table. | Switch between uniform parameter stepping and constant-distance stepping, with the same overall lap duration. Show a speed readout and use a route with visibly different segment shapes. |
| Shadows from multiple light sources | Both the sun and showcase spotlight cast reactor and drone shadows onto nearby surfaces. Different light directions separate their shadows. | Provide independent light and shadow toggles for each source. Pause the drone in the spotlight beam, demonstrate each source alone, then both together. Each shadow attenuates only its own light's contribution. |
| Particle effects | Energy motes rise, drift, and fade around the cyan core, mostly against the dark recess. | Toggle particles independently of bloom. Their animation uses elapsed time so emission and lifetimes do not depend on rendering framerate. |
| Post-processing: bloom | Bright core, lamp face, and selected particles have a soft halo without washing out the white housing. | Toggle bloom at fixed exposure and camera position. The emissive surfaces stay bright with bloom off, while the surrounding halo disappears. |

All features run together in the same application. The UI also provides a global
pause/resume control and a restart control so comparisons can use identical
animation states. Feature comparisons should keep unrelated settings unchanged.

### Possible extensions

The initial target is the mandatory features and the four extras above. If time
allows after those are working and visually polished:

- **Ambient occlusion:** first candidate; subtle contact shading around supports,
  panel recesses, and drone joints, demonstrated with an independent toggle.
- **Minimap:** a simultaneous overhead inset showing the drone and route, rendered
  to a texture rather than only switching to the existing overhead camera.
- **Procedural water:** a shallow surrounding pool with shader-driven waves;
  would require revisiting the platform composition and water material.
- **Other ideas to revisit:** parameter-driven props, a genuinely frame-animated
  diagnostic texture, or coordinated changing lighting conditions.
