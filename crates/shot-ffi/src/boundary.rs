use std::cell::RefCell;
use std::panic::{AssertUnwindSafe, catch_unwind};

pub(crate) type Result<T> = std::result::Result<T, String>;
thread_local! { pub(crate) static ERROR: RefCell<String> = const { RefCell::new(String::new()) }; }

/// 执行 operation 并隔离 panic；成功返回 0，参数或业务错误返回 1，panic 返回 2。
pub(crate) fn run(operation: impl FnOnce() -> Result<()>) -> i32 {
    match catch_unwind(AssertUnwindSafe(operation)) {
        Ok(Ok(())) => {
            ERROR.with(|e| e.borrow_mut().clear());
            0
        }
        Ok(Err(message)) => {
            ERROR.with(|e| *e.borrow_mut() = message);
            1
        }
        Err(_) => {
            ERROR.with(|e| *e.borrow_mut() = "Rust core panicked; operation was rejected".into());
            2
        }
    }
}

/// 执行 operation 并返回其值；异常时返回 fallback，同时保存线程内错误。
pub(crate) fn value<T>(fallback: T, operation: impl FnOnce() -> Result<T>) -> T {
    let mut result = None;
    run(|| {
        result = Some(operation()?);
        Ok(())
    });
    result.unwrap_or(fallback)
}

/// 验证 pointer 的非空和对齐属性；返回指针或错误，不读取指针内容。
pub(crate) fn validate<T>(pointer: *const T) -> Result<()> {
    if pointer.is_null() || (pointer as usize) % std::mem::align_of::<T>() != 0 {
        Err("null or misaligned FFI pointer".into())
    } else {
        Ok(())
    }
}

/// 借用 pointer 起的 len 个元素；返回切片或可验证的布局错误。
/// # Safety
/// 非零 len 必须对应有效且已初始化的连续内存，生命周期内不能释放或修改。
pub(crate) unsafe fn input<'a, T>(pointer: *const T, len: usize) -> Result<&'a [T]> {
    if len == 0 {
        return Ok(&[]);
    }
    validate(pointer)?;
    if len > isize::MAX as usize / std::mem::size_of::<T>() {
        return Err("FFI slice length overflow".into());
    }
    // SAFETY: 【Rust接口】【输入借用】调用者保证内存有效，长度上限和对齐已检查
    Ok(unsafe { std::slice::from_raw_parts(pointer, len) })
}

/// 将 value 写入 output；返回可验证的指针错误。
/// # Safety
/// output 必须指向一个可写 T 的独立存储位置。
pub(crate) unsafe fn write<T>(output: *mut T, value: T) -> Result<()> {
    validate(output)?;
    // SAFETY: 【Rust接口】【结果输出】输出指针对齐且非空，宿主保证存储有效
    unsafe {
        output.write(value);
    }
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    /// 验证 panic 转换为错误码且下一次正常调用可以恢复，无参数、无返回值。
    fn panic_is_contained() {
        assert_eq!(run(|| panic!("boundary fixture")), 2);
        assert!(ERROR.with(|error| error.borrow().contains("panicked")));
        assert_eq!(run(|| Ok(())), 0);
        assert!(ERROR.with(|error| error.borrow().is_empty()));
    }
}
