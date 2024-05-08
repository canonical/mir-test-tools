import 'dart:ui';

/// Defines the anchor point for the anchor rectangle or child view when
/// positioning a View. The specified anchor is used to derive an anchor
/// point on the anchor rectangle that the anchor point for the child View
/// will be positioned relative to. If a corner anchor is set (e.g. [topLeft]
/// or [bottomRight]), the anchor point will be at the specified corner;
/// otherwise, the derived anchor point will be centered on the specified edge,
/// or in the center of the anchor rectangle if no edge is specified.
enum FlutterViewPositionerAnchor {
  center,
  top,
  bottom,
  left,
  right,
  topLeft,
  bottomLeft,
  topRight,
  bottomRight,
}

/// The constraintAdjustment value define ways Flutter will adjust
/// the position of the View, if the unadjusted position would result
/// in the surface being partly constrained.
///
/// Whether a View is considered 'constrained' is left to the platform
/// to determine. For example, the surface may be partly outside the
/// compositor's defined 'work area', thus necessitating the child View's
/// position be adjusted until it is entirely inside the work area.
///
/// "Flip" means reverse the anchor points and offset along an axis.
/// "Slide" means adjust the offset along an axis.
/// "Resize" means adjust the client window size along an axis.
///
/// The adjustments can be combined, according to a defined precedence: 1)
/// Flip, 2) Slide, 3) Resize.
enum FlutterViewPositionerConstraintAdjustment {
  slideX,
  slideY,
  flipX,
  flipY,
  resizeX,
  resizeY,
}

/// The FlutterViewPositioner provides a collection of rules for the placement
/// of a child View relative to a parent View. Rules can be defined to ensure
/// the child View remains within the visible area's borders, and to
/// specify how the child View changes its position, such as sliding along
/// an axis, or flipping around a rectangle.
class FlutterViewPositioner {
  /// Const constructor for [FlutterViewPositioner].
  const FlutterViewPositioner({
    this.parentAnchor = FlutterViewPositionerAnchor.center,
    this.childAnchor = FlutterViewPositionerAnchor.center,
    this.offset = Offset.zero,
    this.constraintAdjustment = const {},
  });

  /// Copy a [FlutterViewPositioner] with some fields replaced.`
  FlutterViewPositioner copyWith({
    FlutterViewPositionerAnchor? parentAnchor,
    FlutterViewPositionerAnchor? childAnchor,
    Offset? offset,
    Set<FlutterViewPositionerConstraintAdjustment>? constraintAdjustment,
  }) {
    return FlutterViewPositioner(
      parentAnchor: parentAnchor ?? this.parentAnchor,
      childAnchor: childAnchor ?? this.childAnchor,
      offset: offset ?? this.offset,
      constraintAdjustment: constraintAdjustment ?? this.constraintAdjustment,
    );
  }

  /// Defines the anchor point for the anchor rectangle. The specified anchor
  /// is used to derive an anchor point that the child View will be
  /// positioned relative to. If a corner anchor is set (e.g. [topLeft] or
  /// [bottomRight]), the anchor point will be at the specified corner;
  /// otherwise, the derived anchor point will be centered on the specified
  /// edge, or in the center of the anchor rectangle if no edge is specified.
  final FlutterViewPositionerAnchor parentAnchor;

  /// Defines the anchor point for the child view. The specified anchor
  /// is used to derive an anchor point that will be positioned relative to the
  /// parentAnchor. If a corner anchor is set (e.g. [topLeft] or
  /// [bottomRight]), the anchor point will be at the specified corner;
  /// otherwise, the derived anchor point will be centered on the specified
  /// edge, or in the center of the anchor rectangle if no edge is specified.
  final FlutterViewPositionerAnchor childAnchor;

  /// Specify the View position offset relative to the position of the
  /// anchor on the anchor rectangle and the anchor on the child. For
  /// example if the anchor of the anchor rectangle is at (x, y), the View
  /// has the child_anchor [topLeft], and the offset is (ox, oy), the calculated
  /// View position will be (x + ox, y + oy). The offset position of the
  /// View is the one used for constraint testing. See constraintAdjustment.
  ///
  /// An example use case is placing a popup menu on top of a user interface
  /// element, while aligning the user interface element of the parent View
  /// with some user interface element placed somewhere in the popup View.
  final Offset offset;

  /// The constraintAdjustment value define ways Flutter will adjust
  /// the position of the View, if the unadjusted position would result
  /// in the surface being partly constrained.
  ///
  /// Whether a View is considered 'constrained' is left to the platform
  /// to determine. For example, the surface may be partly outside the
  /// output's 'work area', thus necessitating the child View's
  /// position be adjusted until it is entirely inside the work area.
  ///
  /// The adjustments can be combined, according to a defined precedence: 1)
  /// Flip, 2) Slide, 3) Resize.
  final Set<FlutterViewPositionerConstraintAdjustment> constraintAdjustment;
}
