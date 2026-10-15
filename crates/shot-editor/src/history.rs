use crate::Snapshot;
use std::collections::VecDeque;

const MEMORY_LIMIT: usize = 128 * 1024 * 1024;

pub struct History {
    undo: VecDeque<Snapshot>,
    redo: VecDeque<Snapshot>,
    pending: Option<(i32, Snapshot)>,
    limit: usize,
}

impl History {
    /// 按 limit 创建独立历史，返回容量有效的会话；limit 必须在 1 至 4096 内。
    pub fn new(limit: usize) -> Result<Self, &'static str> {
        if !(1..=4096).contains(&limit) {
            return Err("invalid history capacity");
        }
        Ok(Self {
            undo: VecDeque::new(),
            redo: VecDeque::new(),
            pending: None,
            limit,
        })
    }
    /// 返回撤销数量、重做数量与挂起上下文，无参数。
    pub fn info(&self) -> (usize, usize, Option<i32>) {
        (
            self.undo.len(),
            self.redo.len(),
            self.pending.as_ref().map(|(context, _)| *context),
        )
    }
    /// 清除全部栈及待合并事务，无参数、无返回值。
    pub fn clear(&mut self) {
        self.undo.clear();
        self.redo.clear();
        self.pending = None;
    }

    /// 判断 context 是否需要新的编辑前快照，返回布尔值，供宿主跳过重复序列化。
    pub fn needs_snapshot(&self, context: i32) -> bool {
        self.pending
            .as_ref()
            .is_none_or(|(existing, _)| *existing != context)
    }

    /// 添加 snapshot 作为编辑前状态，返回校验结果；已有待合并事务先提交。
    pub fn checkpoint(&mut self, snapshot: Snapshot) -> Result<(), &'static str> {
        snapshot.validate()?;
        self.commit();
        self.undo.push_back(snapshot);
        self.redo.clear();
        self.trim();
        Ok(())
    }

    /// 【编辑核心】【滚轮事务】保存 context 的首个 snapshot，新上下文先提交旧事务，返回校验结果。
    pub fn queue(&mut self, context: i32, snapshot: Snapshot) -> Result<(), &'static str> {
        if !self.needs_snapshot(context) {
            return Ok(());
        }
        snapshot.validate()?;
        self.commit();
        self.pending = Some((context, snapshot));
        self.trim();
        Ok(())
    }

    /// 提交待合并事务并清除重做分支，无参数，返回是否有事务提交。
    pub fn commit(&mut self) -> bool {
        if let Some((_, snapshot)) = self.pending.take() {
            self.undo.push_back(snapshot);
            self.redo.clear();
            self.trim();
            true
        } else {
            false
        }
    }

    /// 查询下一次 step 的结果，redo 指定方向，返回候选引用；不改变任何栈。
    pub fn candidate(&self, redo: bool) -> Option<&Snapshot> {
        if redo {
            if self.pending.is_some() {
                None
            } else {
                self.redo.back()
            }
        } else {
            self.pending
                .as_ref()
                .map(|(_, value)| value)
                .or_else(|| self.undo.back())
        }
    }

    /// 【编辑核心】【历史切换】保存 current 并按 redo 执行方向切换，返回目标快照；输入无效时不修改历史。
    pub fn step(
        &mut self,
        current: Snapshot,
        redo: bool,
    ) -> Result<Option<Snapshot>, &'static str> {
        current.validate()?;
        self.commit();
        let (source, target) = if redo {
            (&mut self.redo, &mut self.undo)
        } else {
            (&mut self.undo, &mut self.redo)
        };
        let Some(result) = source.pop_back() else {
            return Ok(None);
        };
        target.push_back(current);
        self.trim();
        Ok(Some(result))
    }

    /// 按数量及内存预算移除较远快照，无参数、无返回值，保留最近候选与待合并事务。
    fn trim(&mut self) {
        while self.undo.len() > self.limit {
            self.undo.pop_front();
        }
        while self.redo.len() > self.limit {
            self.redo.pop_front();
        }
        let mut bytes = self
            .undo
            .iter()
            .chain(&self.redo)
            .map(Snapshot::bytes)
            .sum::<usize>()
            + self.pending.as_ref().map_or(0, |(_, value)| value.bytes());
        while bytes > MEMORY_LIMIT {
            let removed = if self.undo.len() > 1 || self.redo.is_empty() {
                self.undo.pop_front()
            } else {
                self.redo.pop_front()
            };
            let Some(removed) = removed else {
                break;
            };
            bytes -= removed.bytes();
        }
    }
}
