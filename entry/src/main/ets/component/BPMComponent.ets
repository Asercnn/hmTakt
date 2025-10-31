import { AnimatorResult } from '@ohos.animator';
const TAG:string = "BPMComponent"
@Component
export default struct BPMComponent {
  @Prop BPM:number;
  @Prop bat:number;
  @Prop quarter:number; // 动画刻
  @Prop @Watch("start")isStart:boolean; // 是否开始播放

  CirW:number = 300;  // 圆宽
  CirH:number = 300;  // 圆高

  start(){
    if (this.isStart) {

    }
  }

  /**
   * 获取角度
   * @param deg
   * @returns
   */
  getRad(deg){
    return Math.PI/180 * deg;
  }

  /**
   * 获取x坐标
   * @param deg
   * @returns
   */
  getX(deg:number){
    return (this.CirW/2)*Math.cos(this.getRad(deg))
  }
  /**
   * 获取y坐标
   * @param deg
   * @returns
   */
  getY(deg:number){
    return (this.CirH/2)*Math.sin(this.getRad(deg))
  }



  build() {
    Stack(){
      Circle({width:this.CirW,height:this.CirH}).stroke($r("app.color.circleStroke")).strokeWidth(2).fill(Color.Transparent)
      Column(){
        Text(`${this.BPM}`)
          .fontColor($r("app.color.fontColor"))
          .fontSize(32)
          .fontWeight(FontWeight.Bold)
        Text("BPM")
          .fontColor($r("app.color.fontColor"))
      }
      .alignItems(HorizontalAlign.Center)
      ForEach(Array.from({length:this.bat},(_,i)=>i*(360/this.bat)),(deg)=>{
        Circle({width:20,height:20}).fill("#3c3c53")
          .translate({x:this.getX(deg),y:this.getY(deg)})
          .opacity(this.quarter)
      })
    }
  }
}