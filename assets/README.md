# Scene assets

## Platform — first version

![Platform preview](previews/platform.png)

A 12 m diameter, 0.38 m tall circular exhibit platform with a polished white
marble top, narrow gold rim, and beveled satin-white base. The shared origin is at
ground level in the center. This is the first authored asset; application-side
loading and rendering are still to be implemented.

### Files

| File | Purpose |
| --- | --- |
| `source/platform.blend` | Editable geometry, bevel modifiers, UVs, materials, and a preview studio setup. |
| `models/platform.obj` | Triangulated platform export: 15,352 triangles, three material groups, UVs, and normals. |
| `models/platform.mtl` | Exported material properties and relative texture references. |
| `textures/marble_basecolor.png` | 2048 × 2048 base-color texture, interpreted as sRGB. |
| `textures/marble_roughness.png` | 2048 × 2048 roughness texture, interpreted as linear/non-color data. |
| `previews/platform.png` | Rendered preview for reviewing the asset. |

### Materials

Map the OBJ's material names to the following application materials. Constant
RGB values below are linear shader values. All three materials have zero
emission.

| Material name | Base color | Metallic | Roughness |
| --- | --- | --- | --- |
| `marble` | `textures/marble_basecolor.png` | 0 | `textures/marble_roughness.png`; authored in the 0.22–0.30 range. |
| `gold` | `(0.83, 0.57, 0.23)` | 1 | 0.21 |
| `platform_base` | `(0.73, 0.75, 0.77)` | 0 | 0.34 |

The marble uses a flat geometric normal; the optional marble normal map is not
needed for this version. UV coordinates cover a 12 m texture span, with a planar
mapping on the top and cylindrical mapping on the thin side. The Blender runtime
material uses the same exported PNGs intended for the application.

### Editing and exporting

- The **`Platform | EXPORT`** collection contains the three asset meshes. Each
  shares the ground-center origin and has identity object transforms.
- The **`Studio | PREVIEW ONLY`** collection contains the camera, softbox lights,
  and floor used for the preview. These are excluded from the OBJ export.
- The Blender source uses meters with **Z up**. The OBJ is exported with **Y up**
  and **−Z forward**, at scale 1, for the application's coordinate convention.
- Select the three meshes in the export collection and export selected objects,
  with modifiers applied, triangulation, normals, UVs, materials, and relative
  texture paths. Keep the editable bevel modifiers in the Blender source.
- The unused-but-preserved **`marble_procedural`** material is the editable source
  graph for rebaking the marble. The mesh material **`marble`** uses the baked
  images. Bake color without lighting/shadows and roughness as non-color data
  over a plane with UVs spanning `(0, 0)` to `(1, 1)`.

### Authorship and verification

Geometry and marble textures were created specifically for this project in
Blender. The marble was authored procedurally and baked to PNGs; no third-party
models, textures, or HDRIs were used for this asset. The preview uses Blender
area lights and a uniform world environment.

The evaluated source meshes were checked for closed/manifold geometry. The OBJ
was re-imported in Blender to check geometry, UVs, and its three material groups;
the MTL's texture references were also checked. The preview shows the baked
textures, rather than relying on the procedural authoring graph.
