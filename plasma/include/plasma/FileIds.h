// Legacy PLA chunk IDs used by Engine.cpp's readers and writers.
#ifndef PLASMA_FILE_IDS_H
#define PLASMA_FILE_IDS_H

namespace plasma {

enum MeshChunk {
    MeshChunk_Name = 1,
    MeshChunk_Face = 2,
    MeshChunk_VertexFlags = 3,
    MeshChunk_Positions = 4,
    MeshChunk_TexCoords = 5,
    MeshChunk_Colors = 6,
    MeshChunk_Texture = 7,
    MeshChunk_TextureTranslation = 8,
    MeshChunk_TextureRotation = 9,
    MeshChunk_TextureDeformation = 10,
    MeshChunk_TexturePivot = 11,
    MeshChunk_Subdivisions = 12,
    MeshChunk_Flags = 14,
    MeshChunk_StrokeColors = 15,
    MeshChunk_StrokeWidths = 16,
    MeshChunk_SmoothWeight = 17,
};

enum CurveChunk {
    CurveChunk_Name = 1,
    CurveChunk_VertexFlags = 2,
    CurveChunk_Positions = 3,
    CurveChunk_TexCoords = 4,
    CurveChunk_Colors = 5,
    CurveChunk_Widths = 6,
    CurveChunk_Subdivisions = 8,
    CurveChunk_Closed = 9,
    CurveChunk_StrokeCap = 10,
    CurveChunk_StrokeSegments = 11,
};

enum WidgetChunk {
    WidgetChunk_Name = 1,
    WidgetChunk_HorizontalAlignment = 2,
    WidgetChunk_VerticalAlignment = 3,
    WidgetChunk_StretchX = 4,
    WidgetChunk_StretchY = 5,
    WidgetChunk_InnerBindPosition = 6,
    WidgetChunk_InnerBindSize = 7,
    WidgetChunk_BindPosition = 10,
    WidgetChunk_BindSize = 11,
    WidgetChunk_FramePosition = 12,
    WidgetChunk_FrameSize = 13,
    WidgetChunk_BindMatrix = 14,
    WidgetChunk_ButtonType = 15,
    WidgetChunk_ScrollDirection = 17,
    WidgetChunk_ScrollStep = 18,
    WidgetChunk_ScrollTargetId = 19,
    WidgetChunk_SliderDirection = 20,
    WidgetChunk_SliderTargetId = 21,
    WidgetChunk_Caption = 22,
};

enum TransformationChunk {
    TransformationChunk_Name = 1,
    TransformationChunk_Translation = 2,
    TransformationChunk_Rotation = 3,
    TransformationChunk_Pivot = 5,
    TransformationChunk_Deformation = 6,
};

enum TextureChunk {
    TextureChunk_Name = 1,
    TextureChunk_Id = 2,
    TextureChunk_PixelFormat = 3,
    TextureChunk_MinFilter = 4,
    TextureChunk_MaxFilter = 5,
    TextureChunk_Width = 6,
    TextureChunk_Height = 7,
    TextureChunk_Pixels = 8,
    TextureChunk_HorizontalWrap = 9,
    TextureChunk_VerticalWrap = 10,
};

enum GenericShapeChunk {
    GenericShapeChunk_Name = 1,
    GenericShapeChunk_Source = 2,
    GenericShapeChunk_Position = 3,
    GenericShapeChunk_Size = 4,
};

}

#endif
