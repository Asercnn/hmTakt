import { preferencesUtil } from "pre_as"
import BPMComponent from '../component/BPMComponent'
import BPMControllerComponent from '../component/BPMControllerComponent';
import Animator, { AnimatorResult } from '@ohos.animator';


@Entry
@Component
struct Index {
  @State bat:number = 4;  // 拍子
  @State BPM:number = 120;  // 节拍
  @State quarter:number = 1; // 动画刻
  @State isStart:boolean = false; // 是否开始
  @Provide animateManager?:AnimatorResult = undefined;  // 动画管理对象
  aboutToAppear(): void {
    this.InitAnimate();
  }
  InitAnimate(){
    this.animateManager = Animator.create({
      duration:300,
      easing:"ease",
      delay:0,
      fill:"forwards",
      direction:'normal',
      iterations:1,
      begin:0,
      end:1
    })
    this.animateManager.onframe = (quarter:number)=>{
      this.quarter = quarter;
    }
  }
  build() {
    Column() {
      GridRow({
        columns:{
          xs:1,
          sm:2
        },
        gutter:{
          x:0,
          y:100
        },
        breakpoints:{
          value:["400vp"]
        },
        direction:GridRowDirection.Row
      }){
        GridCol(){
          BPMComponent({
            bat:this.bat,
            BPM:this.BPM,
            quarter:this.quarter,
            isStart:this.isStart
          })
        }

        GridCol(){
          BPMControllerComponent({
            bat:this.bat,
            BPM:this.BPM,
            isStart:this.isStart,
          })
        }

      }

    }
    .justifyContent(FlexAlign.Center)
    .backgroundColor($r("app.color.dark_gackground"))
    .width('100%')
    .height("100%")
  }
}