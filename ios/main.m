// Diagnostic harness only. Synthetic instructions below are authored for this test.
// Runtime: Bloodborne-native (built on bbport and shadPS4). No game data is included.
#import <UIKit/UIKit.h>
#include "cpu/bbcpu.h"
#include "platform.h"
#include <sys/mman.h>
#include <unistd.h>

@interface ProbeController : UIViewController
@property(nonatomic,strong) UITextView *result;
@property(nonatomic,strong) UIButton *runButton;
@end
@implementation ProbeController
- (void)viewDidLoad {
    [super viewDidLoad];
    self.view.backgroundColor = UIColor.systemBackgroundColor;
    self.result = [UITextView new];
    self.result.editable = NO;
    self.result.font = [UIFont systemFontOfSize:18];
    self.result.text = @"Bloodborne iOS CPU 진단\n\n이 빌드는 게임 실행판이 아닙니다.\n실제 Bloodborne-native CPU 인터프리터에 자체 작성한 짧은 x86-64 명령을 전달하여 결과를 확인합니다.\n\n게임 데이터·게임 그래픽·DLC는 포함하지 않습니다.";
    self.runButton = [UIButton buttonWithType:UIButtonTypeSystem];
    [self.runButton setTitle:@"CPU 인터프리터 검사" forState:UIControlStateNormal];
    [self.runButton addTarget:self action:@selector(runProbe) forControlEvents:UIControlEventTouchUpInside];
    UIStackView *stack = [[UIStackView alloc] initWithArrangedSubviews:@[self.runButton,self.result]];
    stack.axis = UILayoutConstraintAxisVertical;
    stack.spacing = 16;
    stack.translatesAutoresizingMaskIntoConstraints = NO;
    [self.view addSubview:stack];
    UILayoutGuide *safe = self.view.safeAreaLayoutGuide;
    [NSLayoutConstraint activateConstraints:@[
        [stack.topAnchor constraintEqualToAnchor:safe.topAnchor constant:20],
        [stack.bottomAnchor constraintEqualToAnchor:safe.bottomAnchor constant:-20],
        [stack.leadingAnchor constraintEqualToAnchor:safe.leadingAnchor constant:20],
        [stack.trailingAnchor constraintEqualToAnchor:safe.trailingAnchor constant:-20]
    ]];
}
- (void)runProbe {
    self.runButton.enabled = NO;
    self.result.text = @"CPU 검사 중…";
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED,0), ^{
        setenv("BB_JIT","0",1);
        const size_t page=(size_t)sysconf(_SC_PAGESIZE);
        // mov rax,rdi; add rax,rsi; ret. This is a synthetic addition function.
        const unsigned char instructions[]={0x48,0x89,0xf8,0x48,0x01,0xf0,0xc3};
        void *memory=bb_low_map(page,PROT_READ|PROT_WRITE);
        NSString *message;
        if (!memory) {
            message=@"FAIL: CPU 코드용 저주소 메모리를 할당하지 못했습니다.\n\n기기 주소 공간·앱 권한의 추가 검토가 필요합니다.";
        } else {
            memcpy(memory,instructions,sizeof(instructions));
            bbcpu_add_guest_code((uintptr_t)memory,sizeof(instructions));
            const uint64_t arguments[]={19,23};
            uint64_t result=bbcpu_call((uintptr_t)memory,2,arguments);
            message=[NSString stringWithFormat:@"%@: x86-64 덧셈 결과 %llu (기대값 42)\n\n기기: %@\niOS: %@\n\n이 결과는 CPU 인터프리터의 짧은 명령 검사만 의미합니다. 게임 로딩·Metal 렌더러·한국어·DLC·게임 성능은 검증하지 않습니다.",
                result==42 ? @"PASS" : @"FAIL",(unsigned long long)result,
                UIDevice.currentDevice.model,UIDevice.currentDevice.systemVersion];
        }
        dispatch_async(dispatch_get_main_queue(),^{self.result.text=message;});
    });
}
@end

@interface ProbeDelegate : UIResponder <UIApplicationDelegate>
@property(nonatomic,strong) UIWindow *window;
@end
@implementation ProbeDelegate
- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)options {
    self.window=[[UIWindow alloc] initWithFrame:UIScreen.mainScreen.bounds];
    self.window.rootViewController=[ProbeController new];
    [self.window makeKeyAndVisible];
    return YES;
}
@end
int main(int argc,char **argv) {
    @autoreleasepool { return UIApplicationMain(argc,argv,nil,NSStringFromClass(ProbeDelegate.class)); }
}
