#pragma once

namespace plasma {

// Ordinary and string attributes accept only 1/3.
enum AttributeChunk {
    AttributeChunk_Frame = 1,
    AttributeChunk_ArrayLength = 2,
    AttributeChunk_Sequence = 3,
};

enum SequenceChunk {
    SequenceChunk_NameId = 1,
    SequenceChunk_Key = 2,
    SequenceChunk_Name = 3,
};

enum SequenceKeyChunk {
    SequenceKeyChunk_Element = 1,
    SequenceKeyChunk_Frame = 2,
    SequenceKeyChunk_Smoothness = 3,
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

enum GenericShapeChunk {
    GenericShapeChunk_Name = 1,
    GenericShapeChunk_Source = 2,
    GenericShapeChunk_Position = 3,
    GenericShapeChunk_Size = 4,
};

enum MovieChunk {
    MovieChunk_Name = 1,
    MovieChunk_Entry = 2,
};

enum MovieEntryChunk {
    MovieEntryChunk_NameId = 1,
    MovieEntryChunk_Value = 2,
    MovieEntryChunk_Name = 3,
};

enum PlaChunk {
    PlaChunk_Transformation = 1,
    PlaChunk_SmoothMesh = 3,
    PlaChunk_Text = 4,
    PlaChunk_Texture = 5,
    PlaChunk_Font = 6,
    PlaChunk_Node = 7,
    PlaChunk_Movie = 8,
    PlaChunk_Name = 9,
    PlaChunk_Curve = 10,
    PlaChunk_Widget = 11,
    PlaChunk_WidgetAlias = 12,
    PlaChunk_Button = 13,
    PlaChunk_Edit = 14,
    PlaChunk_ScrollButton = 17,
    PlaChunk_ScrollSlider = 18,
    PlaChunk_ListWidget = 19,
    PlaChunk_PageWidth = 20,
    PlaChunk_PageHeight = 21,
    PlaChunk_PageDepth = 22,
    PlaChunk_PageUnit = 23,
    PlaChunk_GenericShape = 24,
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

enum FontChunk {
    FontChunk_Name = 1,
    FontChunk_Source = 2,
    FontChunk_Size = 3,
    FontChunk_Id = 4,
};

// Chunk identifiers named for their payloads.
enum LegacyNodeChunk {
    LegacyNodeChunk_Name = 1,
    LegacyNodeChunk_ParentId = 2,
    LegacyNodeChunk_ChildId = 3,
    LegacyNodeChunk_ShapeId = 4,
    LegacyNodeChunk_TransformationId = 5,
    LegacyNodeChunk_WidgetId = 6,
    LegacyNodeChunk_Flags = 7,
    LegacyNodeChunk_Visibility = 8,
    LegacyNodeChunk_Clipping = 9,
    LegacyNodeChunk_Variable = 10,
    LegacyNodeChunk_Fill = 11,
};

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

enum TextChunk {
    TextChunk_Name = 1,
    TextChunk_Strings = 2,
    TextChunk_Color = 3,
};

enum TransformationChunk {
    TransformationChunk_Name = 1,
    TransformationChunk_Translation = 2,
    TransformationChunk_Rotation = 3,
    TransformationChunk_Pivot = 5,
    TransformationChunk_Deformation = 6,
};

// Destination members and setter names establish labels.
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

enum NameChunk {
    NameChunk_Id = 1,
    NameChunk_Name = 3,
};

}
