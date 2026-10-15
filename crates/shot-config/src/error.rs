#[derive(Debug)]
pub enum Error {
    Io(std::io::Error),
    Json(serde_json::Error),
    Invalid(&'static str),
}
pub type Result<T> = std::result::Result<T, Error>;

impl std::fmt::Display for Error {
    /// 将当前错误写入 formatter，返回格式化结果，不输出配置内容。
    fn fmt(&self, formatter: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Self::Io(e) => e.fmt(formatter),
            Self::Json(e) => e.fmt(formatter),
            Self::Invalid(e) => formatter.write_str(e),
        }
    }
}
impl std::error::Error for Error {}
impl From<std::io::Error> for Error {
    /// 把 error 转为配置存储错误并返回。
    fn from(error: std::io::Error) -> Self {
        Self::Io(error)
    }
}
impl From<serde_json::Error> for Error {
    /// 把 error 转为 JSON 错误并返回。
    fn from(error: serde_json::Error) -> Self {
        Self::Json(error)
    }
}
