# 旋转图像
1. 从图可知,第`i`行对应的位置为第`n-i`列,但是这样做会需要一个额外数组进行存储矩阵的原始值
2. 原地交换的思路:为了实现原地交换,我们可以按从外向内的圈考虑,因为交换的元素肯定在一个圈上.从图像矩阵的旋转可知,任何一个元素对应的是4个元素的交换,因此可用使用一个临时变量来实现圈上的四个元素的原地交换;进一步可知,第`i`圈只需遍历`n-i-1`个元素(从第`i`圈的第一行观察就可清楚);而一个`n*n`的矩阵,一共需遍历`n/2`个圈、
3. 实现一个圈上4个位置的元素的原地交换
   ```C++
    int temp=matrix[i][j];
    matrix[n-j-1][i]=matrix[n-i-1][n-j-1];
    matrix[n-i-1][n-j-1]=matrix[j][n-i-1];
    matrix[j][n-i-1]=temp;
   ```
# 搜索二维矩阵Ⅱ
1. 对于这种由有序数组组成的数据结构,第一反应就是直接用二分法查找
2. 为了进一步降低时间复杂度,我们可以考虑使用`Z字形查找`,基于每行元素从左到右升序排列、m每列元素从上到下升序排列的事实,可以有以下思路:从矩阵的右上角考虑(因为对于右上角处的元素,它若大于target它就只能往下走,往左值会越来越小);若大于target,则会减此时的列标(相当于删去最后一列,再考虑新的矩阵的右上角元素);若小于target也是同理.在搜索过程中,当超出了矩阵边界,则矩阵中不存在target
# 二分法(查找/搜索)
1. 二分法是一种在有序数组或列表中查找目标值的高效算法,该算法的基本思想是将待查找的区间分为两部分，并在每次比较中将查找的区间分为两部分,并在每次比较中将查找范围缩小一半,直到找到目标值或确定目标值不存在为止
2. 二分法具体步骤:
   ```C++
   1. 初始化左右边界left.通常,左边界初始化为数组的起始位置0,边界初始化为数组的结束位置.size()   #形成一个左闭右闭区间
   2. 在每次迭代中,计算中间位置mid,即(left + right) / 2
   3. 如果目标值等于 array[mid],则找到目标值,返回 mid
   4. 如果目标值小于 array[mid],则目标值可能位于左半部分,更新右边界为 mid - 1
   5. 如果目标值大于 array[mid],则目标值可能位于右半部分,更新左边界为 mid + 1
   6. 重复步骤 2 到步骤 5,直到左边界大于右边界为止.如果找到目标值,则返回其索引;否则,返回不存在的指示
   ```
3. 二分法代码:
   ```C++
   1. 按照步骤手撕:
   //二分法
   bool searchMatrix(vector<vector<int>>& matrix, int target) {
      for(auto& i:matrix){//每一行的数组元素
            //定义一个左闭右闭的二分区间,下面是左闭右闭的写法
            int left=0;//二分区间的左端点
            int right=matrix[0].size()-1;//二分区间的右端点
            while(left<=right){
               int middle=(left+right)/2;
               if(i[middle]>target)
                  right=middle-1;
               else if(i[middle]<target)
                  left=middle+1;
               else
                  return true;
            }
            return false;
      }
      2. 利用lower_bound()函数:
      bool searchMatrix(vector<vector<int>>& matrix, int target) {
         for(auto& i:matrix){
            auto t = lower_bound(i.begin(),i.end(),target);//找到第一个不小于target的元素,并返回其迭代器
            if(t!=i.end()&&*i==target)
               return true;
         }
         return false;
      }
      //std::lower_bound()是C++标准库的一个函数,它用于在已排序的序列(数组、向量等)中查找第一个不小于某个值的元素,并返回其迭代器
      ```
4. 二分法的时间复杂度是`O(logn)`