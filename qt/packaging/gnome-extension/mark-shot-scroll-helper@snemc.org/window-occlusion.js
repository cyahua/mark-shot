/**
 * 计算矩形扣除遮挡区域后剩余的部分
 * @param {object} a 原始矩形
 * @param {object} b 遮挡矩形
 * @returns {object[]} 剩余的互不重叠矩形
 */
export function rectangleDifference(a, b) {
    const ix = Math.max(a.x, b.x);
    const iy = Math.max(a.y, b.y);
    const ix2 = Math.min(a.x + a.width, b.x + b.width);
    const iy2 = Math.min(a.y + a.height, b.y + b.height);

    if (ix >= ix2 || iy >= iy2) {
        return [a];
    }

    const pieces = [];
    if (ix > a.x) {
        pieces.push({x: a.x, y: a.y, width: ix - a.x, height: a.height});
    }
    if (ix2 < a.x + a.width) {
        pieces.push({x: ix2, y: a.y, width: a.x + a.width - ix2, height: a.height});
    }
    if (iy > a.y) {
        pieces.push({x: ix, y: a.y, width: ix2 - ix, height: iy - a.y});
    }
    if (iy2 < a.y + a.height) {
        pieces.push({x: ix, y: iy2, width: ix2 - ix, height: a.y + a.height - iy2});
    }
    return pieces;
}

/**
 * 判断窗口是否已被前方窗口完全遮挡
 * @param {object} rect 待检查的窗口矩形
 * @param {object[]} occluders 前方窗口的矩形列表
 * @returns {boolean} 所有区域均被遮挡时返回 true
 */
export function isFullyOccluded(rect, occluders) {
    let remaining = [rect];

    for (const occluder of occluders) {
        const next = [];
        for (const piece of remaining) {
            next.push(...rectangleDifference(piece, occluder));
        }
        remaining = next;
        if (remaining.length === 0) {
            return true;
        }
    }
    return false;
}

