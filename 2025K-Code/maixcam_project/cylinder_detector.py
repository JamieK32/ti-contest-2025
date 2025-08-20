# lab_reliable_detector.py
from maix import image

RED = image.Color.from_rgb(255, 0, 0)
GREEN = image.Color.from_rgb(0, 255, 0)
WHITE = image.Color.from_rgb(255, 255, 255)
YELLOW = image.Color.from_rgb(255, 255, 0)

class LABReliableDetector:
    def __init__(self):
        self.rois = [
            (30, 31, 17, 50),   # 1,1
            (147, 23, 7, 37),  # 1,3
            (201, 10, 15, 63),  # 2,1
            (362, 21, 11, 34),  # 3,3
            (414, 8, 17, 60),   # 3,1
            (386, 15, 9, 43)   # 3,2
        ]
        
        self.last_results = [[0, 0, 0, 0, 0] for _ in range(6)]  # 扩大到5帧
        self.frame_idx = 0
        
        # LAB阈值设置 (L_min, L_max, A_min, A_max, B_min, B_max)
        # 放宽L值范围以提高检测率
        self.black_lab_threshold = (0, 25, -128, 127, -128, 127)
        
        # 动态参数配置
        self.roi_configs = [
            {"pixels_threshold": 20, "area_threshold": 15, "black_threshold": 0.6},  # ROI 0: 17x50
            {"pixels_threshold": 15, "area_threshold": 12, "black_threshold": 0.6}, # ROI 1: 16x35
            {"pixels_threshold": 25, "area_threshold": 20, "black_threshold": 0.6}, # ROI 2: 15x63
            {"pixels_threshold": 10, "area_threshold": 8, "black_threshold": 0.6},   # ROI 3: 11x34
            {"pixels_threshold": 25, "area_threshold": 20, "black_threshold": 0.6}, # ROI 4: 17x60
            {"pixels_threshold": 8, "area_threshold": 6, "black_threshold": 0.6}   # ROI 5: 9x43
        ]
        


    def detect(self, img):
        colors = []
        
        for i, (x, y, w, h) in enumerate(self.rois):
            try:
                # 获取当前ROI的动态参数
                config = self.roi_configs[i]
                
                # 使用动态参数的blob检测
                black_blobs = img.find_blobs([self.black_lab_threshold], 
                                           roi=(x, y, w, h),
                                           pixels_threshold=config["pixels_threshold"],
                                           area_threshold=config["area_threshold"],
                                           merge=True)
                
                roi_area = w * h
                black_area = sum([blob.pixels() for blob in black_blobs]) if black_blobs else 0
                black_ratio = black_area / roi_area
                black_threshold = config["black_threshold"]
                
                if black_ratio > black_threshold:
                    current_result = 1  # 黑色柱子
                else:
                    current_result = 0  # 白色柱子（不是黑色就是白色）
                
                # 时间滤波 - 使用5帧
                self.last_results[i][self.frame_idx % 5] = current_result
                
                # 获取最终结果（众数）
                result_counts = [0, 0, 0]  # [黑色, 白色, 未知]
                for result in self.last_results[i]:
                    result_counts[result] += 1
                
                final_result = result_counts.index(max(result_counts))
                colors.append(final_result)
                
                # 可视化
                if final_result == 1:      # 黑色
                    color = RED
                elif final_result == 0:    # 白色
                    color = GREEN
                else:                      # 理论上不会到这里
                    color = None
                
                if color:
                    img.draw_rect(x, y, w, h, color=color, thickness=2)
                
                # 在ROI下方显示黑色占比
                ratio_text = f"{black_ratio:.2f}"
                # 对于ROI 3 (3,3位置)，文字位置向左偏移
                if i == 3:  # (358, 12, 16, 40) 是第4个ROI，索引为3
                    text_x = x + w // 2 - 35  # 向左偏移更多
                else:
                    text_x = x + w // 2 - 15  # 文字居中对齐
                text_y = y + h + 5        # 在ROI下方5像素处
                
                # 绘制文字背景（可选，提高可读性）
                img.draw_rect(text_x - 2, text_y - 2, 30, 12, 
                             color=WHITE, thickness=-1)  # 填充白色背景
                
                # 绘制黑色占比文字
                img.draw_string(text_x, text_y, ratio_text, 
                               color=RED if final_result == 1 else GREEN, 
                               scale=1)
                
            except Exception as e:
                print(f"ROI {i} error: {e}")
                colors.append(1)  # 错误时默认为白色
        
        self.frame_idx += 1
        
        return colors


    def update_lab_thresholds(self, black_threshold=None):
        """动态调整LAB阈值
        Args:
            black_threshold: (L_min, L_max, A_min, A_max, B_min, B_max)
        """
        if black_threshold:
            self.black_lab_threshold = black_threshold
            
    def get_detection_stats(self):
        """获取检测统计信息"""
        return {
            'black_lab_threshold': self.black_lab_threshold,
            'roi_count': len(self.rois),
            'roi_configs': self.roi_configs,
            'time_filter_window': 5
        }
        
    def debug_roi_colors(self, img, roi_index=0):
        """调试特定ROI的颜色分布"""
        if roi_index >= len(self.rois):
            return
            
        x, y, w, h = self.rois[roi_index]
        
        print(f"ROI {roi_index} LAB analysis:")
        print(f"Black threshold: {self.black_lab_threshold}")
        
        # 查找并打印黑色blob信息
        try:
            black_blobs = img.find_blobs([self.black_lab_threshold], 
                                        roi=(x, y, w, h),
                                        pixels_threshold=5,
                                        area_threshold=5)
            
            print(f"Black blobs found: {len(black_blobs) if black_blobs else 0}")
            
            if black_blobs:
                total_black_pixels = sum([blob.pixels() for blob in black_blobs])
                roi_area = w * h
                black_ratio = total_black_pixels / roi_area
                print(f"Black ratio: {black_ratio:.3f}")
                
                for i, blob in enumerate(black_blobs):
                    print(f"  Black blob {i}: pixels={blob.pixels()}, area={blob.area()}")
            else:
                print("No black blobs found - will be classified as WHITE")
                    
        except Exception as e:
            print(f"Debug error: {e}")