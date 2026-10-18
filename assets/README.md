# Scene assets

## Platform

![Platform preview](previews/platform.png)

A 12 m diameter, 0.38 m tall circular exhibit platform with a polished white
marble top, narrow gold rim, and beveled satin-white base. An original-color
TU Delft logo near the front edge provides an orientation marker and a clearly
visible normal-mapping example. The shared origin is at ground level in the
center. The application loads its three material groups and base-color texture
in the platform viewer, using simple directional diffuse lighting and ambient
fill. The previews below are Blender renders of the full authored materials;
application-side PBR, roughness, normal mapping, and reflections are planned next.

### Files

| File | Purpose |
| --- | --- |
| `source/platform.blend` | Editable platform geometry, UVs, materials, hidden logo-baking sources, and a preview studio with overview/detail cameras. |
| `source/TU_Delft_Logo.svg` | User-provided original vector logo, retaining its black and blue colors. |
| `models/platform.obj` | Triangulated platform export: 15,352 triangles, three material groups, UVs, and normals. |
| `models/platform.mtl` | Exported material properties and relative texture references. |
| `textures/marble_basecolor.png` | Reusable 2048 × 2048 marble-only base color, interpreted as sRGB; source for the platform composition and other marble assets. |
| `textures/marble_roughness.png` | 2048 × 2048 roughness texture, interpreted as linear/non-color data. |
| `textures/platform_basecolor.png` | 4096 × 4096 marble and positioned logo composition, interpreted as sRGB. |
| `textures/platform_normal.png` | 4096 × 4096 tangent-space logo relief, interpreted as linear/non-color data; flat normals elsewhere. |
| `previews/platform.png` | Rendered preview for reviewing the asset. |
| `previews/platform_logo_normal_on.png` | Logo close-up with normal-map strength 1.25. |
| `previews/platform_logo_normal_off.png` | Matching close-up with strength 0; camera, color, lighting, and exposure are unchanged. |

### Materials

The application retains the OBJ's material names and reads their colors and
base-color texture references from the MTL. The table below describes the full
authored materials, including properties for the upcoming PBR renderer. Constant
RGB values are linear shader values. All three materials have zero emission.

| Material name | Base color | Metallic | Roughness | Normal mapping |
| --- | --- | --- | --- | --- |
| `platform_marble` | `textures/platform_basecolor.png` | 0 | `textures/marble_roughness.png`; authored in the 0.22–0.30 range. | `textures/platform_normal.png`, strength 1.25. |
| `gold` | `(0.83, 0.57, 0.23)` | 1 | 0.21 | Geometric normals. |
| `platform_base` | `(0.73, 0.75, 0.77)` | 0 | 0.34 | Geometric normals. |

UV coordinates cover a 12 m texture span, with a planar mapping on the top and
cylindrical mapping on the thin side. The Blender preview material uses the same
exported PNGs intended for the application. The reusable marble-only material
`marble` and its textures remain available separately from `platform_marble`.

Blender exports the RGB normal map reference as
`map_Bump -bm 1.250000 ../textures/platform_normal.png` in the MTL. For this asset,
interpret that image as a **tangent-space normal map**, not a grayscale height
map. Its channel convention is +X, +Y, +Z (OpenGL-style), and its normal-map
strength is 1.25. Use a tangent basis matching the Blender bake.

### Logo placement and normal-map demonstration

- The artwork is approximately 2.2 m wide, centered at Blender `(0, -4.4, 0.38)`
  on the marble top, leaving roughly 1 m to the rim. Its baseline runs along X,
  and it reads upright when viewed from the front.
- The designated front is Blender **−Y**, corresponding to application **+Z**
  after the Y-up OBJ export. The logo stays fixed to the platform's local axes.
- The detailed baking source rises approximately 2.6 cm above the slab, with
  1 cm bevels. The baked effect is deliberately exaggerated for demonstration.
- The runtime platform still has three mesh parts and 15,352 triangles. The logo
  relief is entirely in the normal texture; it adds no runtime logo geometry.
- In Blender's `platform_marble` material, locate the Normal Map node labeled
  **Logo emboss**. Set Strength to **0** for flat artwork and **1.25** for raised
  shading. The normal map changes highlights along the bevels while preserving
  the color image and mesh silhouette.

| Normal mapping on | Normal mapping off |
| --- | --- |
| ![Logo relief enabled](previews/platform_logo_normal_on.png) | ![Flat logo artwork](previews/platform_logo_normal_off.png) |

### Editing and exporting

- The **`Platform | EXPORT`** collection contains the three asset meshes. Each
  shares the ground-center origin and has identity object transforms.
- The **`Studio | PREVIEW ONLY`** collection contains the camera, softbox lights,
  and floor used for the preview, plus a logo detail camera. These are excluded
  from the OBJ export.
- The **`Platform Logo | BAKE SOURCES`** collection is hidden in the viewport and
  renders. It retains the editable SVG curves with extrusion/bevel settings and
  a flat marble backing plane for rebaking. These are excluded from the export.
- The Blender source uses meters with **Z up**. The OBJ is exported with **Y up**
  and **−Z forward**, at scale 1, for the application's coordinate convention.
- Select the three meshes in the export collection and export selected objects,
  with modifiers applied, triangulation, normals, UVs, materials, and relative
  texture paths. Keep the editable bevel modifiers in the Blender source.
- The unused-but-preserved **`marble_procedural`** material is the editable source
  graph for rebaking the marble. The reusable **`marble`** material uses the baked
  marble-only images. Bake color without lighting/shadows and roughness as
  non-color data over a plane with UVs spanning `(0, 0)` to `(1, 1)`.
- For the platform logo textures, evaluate the raised SVG curves to temporary
  meshes and bake them together with the marble backing onto a flat 12 × 12 m
  target plane at slab-top height, with UVs spanning `(0, 0)` to `(1, 1)`. Use
  selected-to-active baking with 0.10 m cage extrusion and 0.20 m maximum ray
  distance. Bake diffuse color with only the Color pass, then tangent-space
  normals with +X/+Y/+Z channels. Keep the high-detail sources out of the export.

### Authorship and verification

Geometry and marble textures were created specifically for this project in
Blender. The marble was authored procedurally and baked to PNGs. The TU Delft logo
artwork is from the user-provided `source/TU_Delft_Logo.svg`, credited to TU Delft;
its beveled relief and platform texture composition were created for the
project. The preview uses Blender area lights and a uniform world environment.

The evaluated source meshes were checked for closed/manifold geometry. The OBJ
was re-imported in Blender to check geometry, UVs, and its three material groups;
the MTL's texture references were also checked. The overview and matched logo
close-ups show the baked textures, rather than the high-detail source geometry.

## Reactor

| Reactor detail | On the platform |
| --- | --- |
| ![Reactor](previews/reactor.png) | ![Reactor on the platform](previews/reactor_on_platform.png) |

A 3.5 m tall static reactor with a 2.49 m diameter footprint: a marble pedestal,
gold collar, three curved ceramic housings, three gold structural supports,
three gold field rings, a dark inner recess, and an emissive cyan energy column.
The crown has radial cooling vents and a luminous upper aperture. A front-facing
ceramic identity panel carries the original-color TU Delft logo, a technical
caption, and shallow recessed channels.

The preview uses the platform as a sizing reference. The application still
loads the platform viewer; the reactor export is ready for a separate model
instance when scene assembly is implemented. Blender previews include the full
PBR materials, emission, and a restrained compositor glow.

### Files

| File | Purpose |
| --- | --- |
| `source/reactor.blend` | Editable reactor parts and bevel modifiers, hidden panel-baking sources, platform reference, preview lighting, and three cameras. |
| `models/reactor.obj` / `models/reactor.mtl` | Triangulated static assembly: 61,820 triangles, six material groups, UVs, and preserved corner normals. |
| `textures/reactor_panel_basecolor.png` | 2048 × 2048 sRGB panel composition: ceramic background, original logo colors, caption, and groove markings. |
| `textures/reactor_panel_normal.png` | 2048 × 2048 linear/non-color tangent-space relief for the logo, caption, and channels. |
| `previews/reactor.png` | Hero render of the reactor. |
| `previews/reactor_on_platform.png` | Full platform composition showing the relative scale. |
| `previews/reactor_panel_normal_on.png` / `previews/reactor_panel_normal_off.png` | Matching panel close-ups with normal-map strength 1.25 and 0. |

The pedestal reuses `marble_basecolor.png` and `marble_roughness.png` at the
platform's 12 m texture span. The logo source is the existing user-provided
`source/TU_Delft_Logo.svg`.

### Materials

All constant RGB values are linear. Materials other than `cyan_core` have zero
emission. The material names and image references are exported in the MTL;
assign materials by name rather than by mesh order.

| Material | Base color | Metallic | Roughness | Normal mapping |
| --- | --- | --- | --- | --- |
| `marble` | Shared `marble_basecolor.png` | 0 | Shared `marble_roughness.png` | Geometric normals. |
| `gold` | `(0.83, 0.57, 0.23)` | 1 | 0.21 | Geometric normals. |
| `ceramic` | `(0.78, 0.81, 0.84)` | 0 | 0.25 | Geometric normals. |
| `graphite` | `(0.018, 0.028, 0.040)` | 0.65 | 0.30 | Geometric normals. |
| `cyan_core` | `(0.008, 0.32, 0.48)` | 0 | 0.22 | Geometric normals. |
| `reactor_panel` | `reactor_panel_basecolor.png` | 0 | 0.28 | `reactor_panel_normal.png`, strength 1.25. |

The core's Blender emission color is `(0, 0.25, 0.45)` with strength 4, exported
as `Ke 0 1 1.8`. Importing the model into the application will require emission
support to reproduce its luminous appearance.

As with the platform, the panel's MTL reference is
`map_Bump -bm 1.250000 ../textures/reactor_panel_normal.png`. Interpret this as a
**tangent-space normal map**, with OpenGL-style +X, +Y, +Z channels, rather than
a grayscale height image. The panel front uses U along Blender +X and V along
Blender +Z.

| Normal mapping on | Normal mapping off |
| --- | --- |
| ![Reactor relief enabled](previews/reactor_panel_normal_on.png) | ![Flat reactor artwork](previews/reactor_panel_normal_off.png) |

### Scene organization and placement

- **`Reactor | EXPORT`** contains 77 editable mesh parts. Their object transforms
  are identity, and their shared bottom-center origin is `(0, 0, 0)`. The prepared
  OBJ consolidates their evaluated triangles into six material groups.
- **`Reference | Platform`** contains a preview-only import of `platform.obj`,
  lowered by 0.38 m so its top is at Blender Z = 0. It is excluded from the reactor
  export.
- **`Studio | PREVIEW ONLY`** contains the floor, three softboxes, a small cyan
  spill light, and hero, panel-detail, and platform-overview cameras. These are
  excluded from the export. The compositor glow is preview-only too.
- **`Reactor Panel | BAKE SOURCES`** is hidden in the viewport and renders. It
  preserves the editable SVG curves, caption, recessed backing surface, evaluated
  source meshes, and planar UV receiver for rebaking. These are excluded from
  the export.
- The source uses meters with **Z up**, and its designated front is **−Y**. The
  OBJ uses **Y up** and **−Z forward**, so the front becomes application **+Z**.
  Place the reactor on the existing platform with a model translation of
  **`(0, 0.38, 0)`** in application coordinates.

### Baking and verification

The panel's logo source reaches approximately 1.6 cm above the receiver and has
3 mm bevels. The backing channels are approximately 4.5 mm deep. This relief is
stored in the normal texture; the export contains no separate lettering mesh.
Both panel images were baked selected-to-active at 2048 × 2048 with 0.04 m cage
extrusion, 0.08 m maximum ray distance, and a 12-pixel margin. Color uses the
diffuse color pass without direct or indirect lighting; normals use tangent
space with +X, +Y, +Z channels.

All 77 evaluated reactor source meshes were checked for closed/manifold geometry.
The OBJ was re-imported to check its 61,820 triangles, six material assignments,
UVs, custom normals, and 2.49 × 2.49 × 3.5 m bounds. The panel comparison renders
use the exported PNGs with identical camera and lighting. The model and textures
were created for this project in Blender; the logo artwork is credited to TU Delft
and comes from the user-provided SVG.
