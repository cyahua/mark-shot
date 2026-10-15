/// 使用半开边界的整数矩形；width、height 不包含额外的边界像素。
#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct Rect {
    pub x: i32,
    pub y: i32,
    pub width: i32,
    pub height: i32,
}

impl Rect {
    /// 判断矩形是否具有正面积；无参数，返回有效性。
    pub fn valid(self) -> bool {
        self.width > 0 && self.height > 0
    }

    /// 计算与 other 的交集，使用宽整数避免负屏幕坐标溢出；无交集返回空矩形。
    pub fn intersect(self, other: Self) -> Self {
        if !self.valid() || !other.valid() {
            return Self::default();
        }
        let x = self.x.max(other.x);
        let y = self.y.max(other.y);
        let right = (i64::from(self.x) + i64::from(self.width))
            .min(i64::from(other.x) + i64::from(other.width));
        let bottom = (i64::from(self.y) + i64::from(self.height))
            .min(i64::from(other.y) + i64::from(other.height));
        if right <= i64::from(x) || bottom <= i64::from(y) {
            return Self::default();
        }
        Self {
            x,
            y,
            width: (right - i64::from(x)) as i32,
            height: (bottom - i64::from(y)) as i32,
        }
    }
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct Size {
    pub width: i32,
    pub height: i32,
}

impl Size {
    /// 将尺寸转换为原点矩形；无参数，返回对应像素范围。
    pub fn bounds(self) -> Rect {
        Rect {
            x: 0,
            y: 0,
            width: self.width,
            height: self.height,
        }
    }
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct Point {
    pub x: f64,
    pub y: f64,
}

impl Point {
    /// 判断坐标是否有限；无参数，返回是否可参与几何计算。
    pub fn finite(self) -> bool {
        self.x.is_finite() && self.y.is_finite()
    }
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct RectF {
    pub x: f64,
    pub y: f64,
    pub width: f64,
    pub height: f64,
}

impl RectF {
    /// 根据两个角 a、b 创建归一化矩形；非有限坐标返回空矩形。
    pub fn from_corners(a: Point, b: Point) -> Self {
        if !a.finite() || !b.finite() {
            return Self::default();
        }
        Self {
            x: a.x.min(b.x),
            y: a.y.min(b.y),
            width: (a.x - b.x).abs(),
            height: (a.y - b.y).abs(),
        }
    }

    /// 返回归一化矩形；无参数，负尺寸按两个端点转换。
    pub fn normalized(self) -> Self {
        Self::from_corners(
            Point {
                x: self.x,
                y: self.y,
            },
            self.bottom_right(),
        )
    }

    /// 返回右下端点；无参数，不对负尺寸进行修正。
    pub fn bottom_right(self) -> Point {
        Point {
            x: self.x + self.width,
            y: self.y + self.height,
        }
    }

    /// 检查矩形及其端点是否有限；无参数，返回有效性。
    pub fn finite(self) -> bool {
        Point {
            x: self.x,
            y: self.y,
        }
        .finite()
            && self.bottom_right().finite()
    }
}

#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct Screen {
    pub geometry: Rect,
    pub dpr: f64,
}
