// Shared with the portable entry point and included by WindowsUI.cpp.
struct SheetTitleLess {
    bool operator()(Sheet *left, Sheet *right) const { return left->getTitle().compare(right->getTitle()) < 0; }
};
